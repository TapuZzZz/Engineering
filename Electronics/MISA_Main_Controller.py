# =================================================================
#  MISA MAIN CONTROLLER - Camera receive (via mDNS) + FACE detection
#  + motor commands to ESP32 DevKit (via mDNS) + live web dashboard.
#  Laser fires ONLY while spacebar is held down by a human.
# =================================================================

import socket
import struct
import time
import json
import threading
from datetime import datetime, timezone

import numpy as np
import cv2

from fastapi import FastAPI, WebSocket
from fastapi.responses import HTMLResponse
from fastapi.staticfiles import StaticFiles
import uvicorn

# -----------------------------------------------------------------
#  Configuration
# -----------------------------------------------------------------
CAMERA_HOST = "esp32cam.local"
CAMERA_PORT = 8000

MOTOR_ESP_HOST = "esp32motors.local"
MOTOR_ESP_PORT = 9000

DASHBOARD_PORT = 8080   # Separate from camera(8000) and motors(9000)

FRAME_WIDTH  = 640
FRAME_HEIGHT = 480

# -----------------------------------------------------------------
#  Servo safety limits - MUST MATCH the ESP32-S3 Actuation Controller firmware
# -----------------------------------------------------------------
PAN_MIN_ANGLE, PAN_MAX_ANGLE   = 20.0, 160.0
TILT_MIN_ANGLE, TILT_MAX_ANGLE = 60.0, 150.0

# -----------------------------------------------------------------
#  Direction correction
# -----------------------------------------------------------------
PAN_DIRECTION  = 1.0
TILT_DIRECTION = -1.0

# -----------------------------------------------------------------
#  Control tuning - PI controller (Proportional + Integral)
# -----------------------------------------------------------------
PIXELS_TO_DEGREES_GAIN = 0.012
INTEGRAL_GAIN          = 0.0004
INTEGRAL_MAX           = 150.0
MAX_COMMAND_STEP_PER_FRAME = 4.5

FRAMES_WITHOUT_DETECTION_BEFORE_SCAN = 15
SCAN_STEP_DEGREES = 0.3
SCAN_PAN_MIN = 40.0
SCAN_PAN_MAX = 140.0

# -----------------------------------------------------------------
#  Detection filtering
# -----------------------------------------------------------------
MAX_JUMP_PIXELS = 120.0
FRAMES_LOST_BEFORE_ACCEPTING_JUMP = 5
SMOOTHING_ALPHA = 0.4

# -----------------------------------------------------------------
#  Tracking state
# -----------------------------------------------------------------
integral_error_x = 0.0
integral_error_y = 0.0
scan_direction = 1

smoothed_center_x = None
smoothed_center_y = None
frames_since_accepted_detection = 999

# -----------------------------------------------------------------
#  Shared dashboard state - read by the WebSocket, written by the
#  main tracking loop.
# -----------------------------------------------------------------
system_state = {
    "status": "starting",
    "pan": 90.0,
    "tilt": 90.0,
    "laser": 0,
    "locked": 0,
    "fps": 0.0,
    "camera_connected": False,
    "motor_connected": False,
    "frame_count": 0,
    "last_update": None,
}

LOG_FILE_PATH = "turret_events.jsonl"


# -----------------------------------------------------------------
#  Function: log_event
#  What it does: appends one JSON line per event to a local log
#  file - a deliberate JSONL choice, not a database, so each line
#  maps directly to one future database row when needed.
# -----------------------------------------------------------------
def log_event(event_name, **details):
    entry = {
        "time": datetime.now(timezone.utc).isoformat(),
        "event": event_name,
        **details,
    }
    with open(LOG_FILE_PATH, "a") as log_file:
        log_file.write(json.dumps(entry) + "\n")


# -----------------------------------------------------------------
#  Function: load_face_detector
# -----------------------------------------------------------------
def load_face_detector():
    cascade_path = cv2.data.haarcascades + "haarcascade_frontalface_default.xml"
    detector = cv2.CascadeClassifier(cascade_path)

    if detector.empty():
        raise RuntimeError("Failed to load face cascade - opencv-python installation may be incomplete.")

    return detector


# -----------------------------------------------------------------
#  Function: receive_exact
# -----------------------------------------------------------------
def receive_exact(connection, num_bytes):
    data = b""
    while len(data) < num_bytes:
        chunk = connection.recv(num_bytes - len(data))
        if not chunk:
            raise ConnectionError("Camera connection closed while receiving data.")
        data += chunk
    return data


# -----------------------------------------------------------------
#  Function: receive_one_frame
# -----------------------------------------------------------------
def receive_one_frame(connection):
    size_bytes = receive_exact(connection, 4)
    frame_size = struct.unpack("<I", size_bytes)[0]
    jpeg_bytes = receive_exact(connection, frame_size)
    return jpeg_bytes


# -----------------------------------------------------------------
#  Function: decode_frame
# -----------------------------------------------------------------
def decode_frame(jpeg_bytes):
    jpeg_array = np.frombuffer(jpeg_bytes, dtype=np.uint8)
    image = cv2.imdecode(jpeg_array, cv2.IMREAD_COLOR)

    if image is None:
        return None

    return image


# -----------------------------------------------------------------
#  Function: find_best_face_raw
# -----------------------------------------------------------------
def find_best_face_raw(detector, image):
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

    faces = detector.detectMultiScale(
        gray,
        scaleFactor=1.1,
        minNeighbors=5,
        minSize=(40, 40)
    )

    if len(faces) == 0:
        return None

    best_face = max(faces, key=lambda f: f[2] * f[3])
    x, y, w, h = best_face

    return (x, y, x + w, y + h)


# -----------------------------------------------------------------
#  Function: filter_detection
# -----------------------------------------------------------------
def filter_detection(raw_box):
    global smoothed_center_x, smoothed_center_y, frames_since_accepted_detection

    if raw_box is None:
        frames_since_accepted_detection += 1
        return None

    x1, y1, x2, y2 = raw_box
    raw_center_x = (x1 + x2) / 2
    raw_center_y = (y1 + y2) / 2

    if smoothed_center_x is None:
        smoothed_center_x = raw_center_x
        smoothed_center_y = raw_center_y
        frames_since_accepted_detection = 0
        return raw_box

    jump_distance = ((raw_center_x - smoothed_center_x) ** 2 +
                      (raw_center_y - smoothed_center_y) ** 2) ** 0.5

    is_suspicious_jump = jump_distance > MAX_JUMP_PIXELS
    target_was_lost_for_a_while = frames_since_accepted_detection >= FRAMES_LOST_BEFORE_ACCEPTING_JUMP

    if is_suspicious_jump and not target_was_lost_for_a_while:
        frames_since_accepted_detection += 1
        return None

    smoothed_center_x = (SMOOTHING_ALPHA * raw_center_x) + ((1 - SMOOTHING_ALPHA) * smoothed_center_x)
    smoothed_center_y = (SMOOTHING_ALPHA * raw_center_y) + ((1 - SMOOTHING_ALPHA) * smoothed_center_y)
    frames_since_accepted_detection = 0

    half_width  = (x2 - x1) / 2
    half_height = (y2 - y1) / 2
    return (int(smoothed_center_x - half_width), int(smoothed_center_y - half_height),
            int(smoothed_center_x + half_width), int(smoothed_center_y + half_height))


# -----------------------------------------------------------------
#  Function: clamp
# -----------------------------------------------------------------
def clamp(value, min_value, max_value):
    return max(min_value, min(value, max_value))


# -----------------------------------------------------------------
#  Function: rate_limit_step
# -----------------------------------------------------------------
def rate_limit_step(previous_value, requested_value, max_step):
    delta = requested_value - previous_value
    if delta > max_step:
        return previous_value + max_step
    if delta < -max_step:
        return previous_value - max_step
    return requested_value


# -----------------------------------------------------------------
#  Function: compute_tracking_command
# -----------------------------------------------------------------
def compute_tracking_command(box, current_pan, current_tilt):
    global integral_error_x, integral_error_y

    x1, y1, x2, y2 = box

    target_center_x = (x1 + x2) / 2
    target_center_y = (y1 + y2) / 2

    frame_center_x = FRAME_WIDTH / 2
    frame_center_y = FRAME_HEIGHT / 2

    error_x = target_center_x - frame_center_x
    error_y = target_center_y - frame_center_y

    pan_pinned_high = (current_pan >= PAN_MAX_ANGLE) and (error_x * -PAN_DIRECTION < 0)
    pan_pinned_low  = (current_pan <= PAN_MIN_ANGLE) and (error_x * -PAN_DIRECTION > 0)

    if not (pan_pinned_high or pan_pinned_low):
        integral_error_x = clamp(integral_error_x + error_x, -INTEGRAL_MAX, INTEGRAL_MAX)

    tilt_pinned_high = (current_tilt >= TILT_MAX_ANGLE) and (error_y * -TILT_DIRECTION < 0)
    tilt_pinned_low  = (current_tilt <= TILT_MIN_ANGLE) and (error_y * -TILT_DIRECTION > 0)

    if not (tilt_pinned_high or tilt_pinned_low):
        integral_error_y = clamp(integral_error_y + error_y, -INTEGRAL_MAX, INTEGRAL_MAX)

    proportional_pan  = error_x * PIXELS_TO_DEGREES_GAIN
    proportional_tilt = error_y * PIXELS_TO_DEGREES_GAIN

    integral_pan  = integral_error_x * INTEGRAL_GAIN
    integral_tilt = integral_error_y * INTEGRAL_GAIN

    requested_pan  = current_pan  + PAN_DIRECTION  * (proportional_pan  + integral_pan)
    requested_tilt = current_tilt + TILT_DIRECTION * (proportional_tilt + integral_tilt)

    new_pan  = rate_limit_step(current_pan,  requested_pan,  MAX_COMMAND_STEP_PER_FRAME)
    new_tilt = rate_limit_step(current_tilt, requested_tilt, MAX_COMMAND_STEP_PER_FRAME)

    new_pan  = clamp(new_pan,  PAN_MIN_ANGLE,  PAN_MAX_ANGLE)
    new_tilt = clamp(new_tilt, TILT_MIN_ANGLE, TILT_MAX_ANGLE)

    print(f"[TRACK] error=({error_x:.1f},{error_y:.1f}) -> pan={new_pan:.2f} tilt={new_tilt:.2f}")

    return new_pan, new_tilt


# -----------------------------------------------------------------
#  Function: reset_integral
# -----------------------------------------------------------------
def reset_integral():
    global integral_error_x, integral_error_y
    integral_error_x = 0.0
    integral_error_y = 0.0


# -----------------------------------------------------------------
#  Function: reset_smoothing
# -----------------------------------------------------------------
def reset_smoothing():
    global smoothed_center_x, smoothed_center_y
    smoothed_center_x = None
    smoothed_center_y = None


# -----------------------------------------------------------------
#  Function: compute_scan_command
# -----------------------------------------------------------------
def compute_scan_command(current_pan):
    global scan_direction

    new_pan = current_pan + (SCAN_STEP_DEGREES * scan_direction)

    if new_pan >= SCAN_PAN_MAX:
        new_pan = SCAN_PAN_MAX
        scan_direction = -1
    elif new_pan <= SCAN_PAN_MIN:
        new_pan = SCAN_PAN_MIN
        scan_direction = 1

    new_tilt = 90.0

    return new_pan, new_tilt


# -----------------------------------------------------------------
#  Function: check_laser_key
# -----------------------------------------------------------------
def check_laser_key(key_code):
    SPACEBAR = 32
    return 1 if key_code == SPACEBAR else 0


# -----------------------------------------------------------------
#  Function: send_motor_command
# -----------------------------------------------------------------
def send_motor_command(motor_socket, pan_angle, tilt_angle, laser_state, locked_state):
    command_line = f"{pan_angle:.1f},{tilt_angle:.1f},{laser_state},{locked_state}\n"
    motor_socket.sendall(command_line.encode("utf-8"))
    print(f"[SENT] {pan_angle:.1f},{tilt_angle:.1f},{laser_state},{locked_state}")


# -----------------------------------------------------------------
#  Function: draw_overlay
# -----------------------------------------------------------------
def draw_overlay(image, box, status_text):
    center_x, center_y = FRAME_WIDTH // 2, FRAME_HEIGHT // 2

    cv2.circle(image, (center_x, center_y), 8, (0, 255, 0), 1)
    cv2.line(image, (0, center_y), (FRAME_WIDTH, center_y), (0, 255, 0), 1)
    cv2.line(image, (center_x, 0), (center_x, FRAME_HEIGHT), (0, 255, 0), 1)

    if box is not None:
        x1, y1, x2, y2 = box
        cv2.rectangle(image, (x1, y1), (x2, y2), (0, 0, 255), 2)

    cv2.putText(image, status_text, (10, 25), cv2.FONT_HERSHEY_SIMPLEX,
                0.6, (0, 255, 255), 2)
    cv2.putText(image, "HOLD SPACE TO FIRE", (10, FRAME_HEIGHT - 15),
                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 200, 255), 1)

    cv2.imshow("Tracking View", image)

    key_code = cv2.waitKey(1) & 0xFF
    return key_code


# =================================================================
#  WEB DASHBOARD - FastAPI app, runs in a background thread
# =================================================================

dashboard_app = FastAPI()


@dashboard_app.get("/")
def serve_dashboard_page():
    # CHANGED: filename corrected to match the actual file on disk exactly
    # (Linux is case-sensitive - "dashboard.html" and "MISA_Dashboard.html"
    # are two different files as far as the filesystem is concerned).
    with open("MISA_Dashboard.html", "r", encoding="utf-8") as html_file:
        return HTMLResponse(html_file.read())


@dashboard_app.websocket("/ws")
async def dashboard_websocket(websocket: WebSocket):
    """
    What it does: accepts a browser connection and pushes the
    current system_state as JSON, about 6 times per second, for as
    long as the browser tab stays open.
    """
    await websocket.accept()
    try:
        while True:
            await websocket.send_text(json.dumps(system_state))
            await __import__("asyncio").sleep(0.15)
    except Exception:
        pass   # Browser closed the tab or connection dropped - nothing to clean up


def run_dashboard_server():
    uvicorn.run(dashboard_app, host="0.0.0.0", port=DASHBOARD_PORT, log_level="warning")


# =================================================================
#  MAIN PROGRAM
# =================================================================

def main():
    print("Loading face detector...")
    detector = load_face_detector()
    print("Face detector loaded.")

    # --- Start the web dashboard in the background ---
    dashboard_thread = threading.Thread(target=run_dashboard_server, daemon=True)
    dashboard_thread.start()
    print(f"Dashboard running at http://localhost:{DASHBOARD_PORT}")

    print(f"Connecting to camera at {CAMERA_HOST}...")
    camera_connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    camera_connection.connect((CAMERA_HOST, CAMERA_PORT))
    print("Connected to ESP32-S3-CAM.")
    system_state["camera_connected"] = True

    print(f"Connecting to motor controller at {MOTOR_ESP_HOST}...")
    motor_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    motor_socket.connect((MOTOR_ESP_HOST, MOTOR_ESP_PORT))
    print("Connected to motor controller.")
    system_state["motor_connected"] = True

    log_event("system_started")

    current_pan  = 90.0
    current_tilt = 90.0
    frames_without_detection = 0

    frame_counter_for_fps = 0
    fps_window_start_time = time.time()
    was_locked_previously = False

    while True:
        try:
            jpeg_bytes = receive_one_frame(camera_connection)
        except ConnectionError:
            print("Camera connection lost - stopping.")
            system_state["camera_connected"] = False
            log_event("camera_disconnected")
            break

        image = decode_frame(jpeg_bytes)

        if image is None:
            continue

        raw_box = find_best_face_raw(detector, image)
        box = filter_detection(raw_box)

        if box is not None:
            frames_without_detection = 0
            current_pan, current_tilt = compute_tracking_command(box, current_pan, current_tilt)
            status_text = "TRACKING FACE"
        else:
            frames_without_detection += 1
            reset_integral()
            if frames_without_detection >= FRAMES_WITHOUT_DETECTION_BEFORE_SCAN:
                reset_smoothing()
                current_pan, current_tilt = compute_scan_command(current_pan)
                status_text = "SCANNING"
            else:
                status_text = "HOLDING"

        locked_state = 1 if box is not None else 0

        if locked_state == 1 and not was_locked_previously:
            log_event("lock_acquired", pan=round(current_pan, 1), tilt=round(current_tilt, 1))
        elif locked_state == 0 and was_locked_previously:
            log_event("lock_lost")
        was_locked_previously = (locked_state == 1)

        try:
            key_code = draw_overlay(image, box, f"{status_text} | pan={current_pan:.1f} tilt={current_tilt:.1f}")
            laser_state = check_laser_key(key_code)

            if laser_state == 1 and system_state.get("laser") != 1:
                log_event("laser_fired", pan=round(current_pan, 1), tilt=round(current_tilt, 1))

            send_motor_command(motor_socket, current_pan, current_tilt, laser_state, locked_state)
        except (ConnectionError, OSError) as motor_error:
            print(f"Motor command failed (will retry next frame): {motor_error}")
            laser_state = 0

        frame_counter_for_fps += 1
        now = time.time()
        if now - fps_window_start_time >= 1.0:
            system_state["fps"] = round(frame_counter_for_fps / (now - fps_window_start_time), 1)
            frame_counter_for_fps = 0
            fps_window_start_time = now

        system_state["status"] = status_text
        system_state["pan"] = round(current_pan, 1)
        system_state["tilt"] = round(current_tilt, 1)
        system_state["laser"] = laser_state
        system_state["locked"] = locked_state
        system_state["frame_count"] += 1
        system_state["last_update"] = datetime.now(timezone.utc).isoformat()

    camera_connection.close()
    motor_socket.close()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()