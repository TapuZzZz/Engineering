import socket

import cv2
from ultralytics import YOLO

SOURCE = "http://MISA-CAM.local/stream"
MODEL_PATH = "yolo26n.pt"
TARGET_CLASS = "bottle"

ACTIVATOR_HOST = "ESP-ACTIVATOR.local"
ACTIVATOR_PORT = 5006

PAN_DEG_PER_PX = 0.08    # tune: degrees of servo per pixel of error
TILT_DEG_PER_PX = 0.08
PAN_SIGN = 1             # flip to -1 if the servo moves the wrong way
TILT_SIGN = -1
TRACK_GAIN = 0.5         # 1 = full correction per frame, lower = smoother
DEAD_ZONE_PX = 20        # ignore small errors to avoid jitter
START_ANGLE = 90
MIN_ANGLE = 0
MAX_ANGLE = 180


def find_class_id(model, class_name):
    return [i for i, n in model.names.items() if n == class_name][0]


def detect(model, frame, target_id):
    return model.track(frame, persist=True, tracker="bytetrack.yaml",
                       classes=[target_id], device="mps", verbose=False)


def pick_target(results):
    boxes = results[0].boxes
    if len(boxes) == 0:
        return None
    return max(boxes, key=lambda b: float(b.conf[0]))


def box_center(box):
    x1, y1, x2, y2 = box.xyxy[0].tolist()
    return (x1 + x2) / 2, (y1 + y2) / 2


def frame_error(frame, cx, cy):
    h, w = frame.shape[:2]
    return cx - w / 2, cy - h / 2


def update_angle(angle, error_px, deg_per_px, sign):
    if abs(error_px) < DEAD_ZONE_PX:
        return angle
    new_angle = angle + sign * error_px * deg_per_px * TRACK_GAIN
    return max(MIN_ANGLE, min(MAX_ANGLE, new_angle))


def resolve_activator():
    try:
        return socket.gethostbyname(ACTIVATOR_HOST), ACTIVATOR_PORT
    except socket.gaierror:
        print("Activator not found - running without sending")
        return None


def send_angles(sock, address, pan, tilt):
    if address is None:
        return
    message = f"P,{round(pan)},{round(tilt)}"
    sock.sendto(message.encode(), address)


def main():
    cap = cv2.VideoCapture(SOURCE)
    model = YOLO(MODEL_PATH)
    target_id = find_class_id(model, TARGET_CLASS)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    address = resolve_activator()
    pan, tilt = START_ANGLE, START_ANGLE

    while True:
        ok, frame = cap.read()
        if not ok:
            break
        frame = cv2.rotate(frame, cv2.ROTATE_90_CLOCKWISE)
        results = detect(model, frame, target_id)
        box = pick_target(results)
        if box is not None:
            cx, cy = box_center(box)
            err_x, err_y = frame_error(frame, cx, cy)
            pan = update_angle(pan, err_x, PAN_DEG_PER_PX, PAN_SIGN)
            tilt = update_angle(tilt, err_y, TILT_DEG_PER_PX, TILT_SIGN)
            send_angles(sock, address, pan, tilt)
            print(f"err=({err_x:.0f},{err_y:.0f}) pan={pan:.0f} tilt={tilt:.0f}")
        cv2.imshow("cam", results[0].plot())
        if cv2.waitKey(1) == ord("q"):
            break

    cap.release()
    sock.close()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()