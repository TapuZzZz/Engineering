# =================================================================
#  MISA CAMERA-ONLY TEST - temporary version
#  Skips the motor controller connection entirely. Connects to the
#  camera, runs face detection for a visual sanity check, and shows
#  the live "Tracking View" window with FPS - but sends NO commands
#  anywhere and does not touch the DevKit/motors at all.
#
#  Use this to verify the camera + detection pipeline while the
#  DevKit is still being debugged separately. Once the DevKit is
#  working again, go back to the full MISA_Main_Controller.py.
# =================================================================

import socket
import struct
import time

import numpy as np
import cv2

CAMERA_HOST = "esp32cam.local"
CAMERA_PORT = 8000

FRAME_WIDTH  = 640
FRAME_HEIGHT = 480

DETECTION_DOWNSCALE = 0.5


def load_face_detector():
    frontal_path = cv2.data.haarcascades + "haarcascade_frontalface_default.xml"
    profile_path = cv2.data.haarcascades + "haarcascade_profileface.xml"

    frontal_detector = cv2.CascadeClassifier(frontal_path)
    profile_detector = cv2.CascadeClassifier(profile_path)

    if frontal_detector.empty():
        raise RuntimeError("Failed to load frontal face cascade - opencv-python installation may be incomplete.")
    if profile_detector.empty():
        raise RuntimeError("Failed to load profile face cascade - opencv-python installation may be incomplete.")

    return frontal_detector, profile_detector


def receive_exact(connection, num_bytes):
    data = b""
    while len(data) < num_bytes:
        chunk = connection.recv(num_bytes - len(data))
        if not chunk:
            raise ConnectionError("Camera connection closed while receiving data.")
        data += chunk
    return data


def receive_one_frame(connection):
    size_bytes = receive_exact(connection, 4)
    frame_size = struct.unpack("<I", size_bytes)[0]
    return receive_exact(connection, frame_size)


def decode_frame(jpeg_bytes):
    jpeg_array = np.frombuffer(jpeg_bytes, dtype=np.uint8)
    image = cv2.imdecode(jpeg_array, cv2.IMREAD_COLOR)
    if image is None:
        return None
    # Compensates for set_vflip(1) applied on the OV3660 sensor.
    image = cv2.rotate(image, cv2.ROTATE_180)
    return image


_printed_frame_size = False


def find_best_face_raw(frontal_detector, profile_detector, image):
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

    # Histogram equalization boosts local contrast - directly helps
    # in the dim indoor lighting we've been dealing with, where low
    # contrast was likely causing some of the missed detections.
    gray = cv2.equalizeHist(gray)

    small_gray = cv2.resize(gray, None, fx=DETECTION_DOWNSCALE, fy=DETECTION_DOWNSCALE, interpolation=cv2.INTER_AREA)
    scale_back = 1.0 / DETECTION_DOWNSCALE

    # Slightly more sensitive than before (scaleFactor 1.1->1.05,
    # minNeighbors 5->4) - more detection attempts per frame, catches
    # more genuine faces at the cost of a few more false positives
    # and a bit more compute time.
    faces = frontal_detector.detectMultiScale(small_gray, scaleFactor=1.05, minNeighbors=4, minSize=(20, 20))

    if len(faces) > 0:
        best_face = max(faces, key=lambda f: f[2] * f[3])
        x, y, w, h = best_face
        return (int(x * scale_back), int(y * scale_back), int((x + w) * scale_back), int((y + h) * scale_back))

    # Frontal cascade found nothing - try profile detection, which
    # catches turned/angled heads that frontal detection misses.
    # The profile cascade only detects faces looking toward one
    # side, so we check the image as-is AND horizontally flipped to
    # catch both directions.
    profile_faces = profile_detector.detectMultiScale(small_gray, scaleFactor=1.05, minNeighbors=4, minSize=(20, 20))
    if len(profile_faces) > 0:
        best_face = max(profile_faces, key=lambda f: f[2] * f[3])
        x, y, w, h = best_face
        return (int(x * scale_back), int(y * scale_back), int((x + w) * scale_back), int((y + h) * scale_back))

    flipped_gray = cv2.flip(small_gray, 1)
    flipped_faces = profile_detector.detectMultiScale(flipped_gray, scaleFactor=1.05, minNeighbors=4, minSize=(20, 20))
    if len(flipped_faces) > 0:
        best_face = max(flipped_faces, key=lambda f: f[2] * f[3])
        x, y, w, h = best_face
        small_width = small_gray.shape[1]
        # Un-flip the x coordinate back to original orientation.
        flipped_x = small_width - (x + w)
        return (int(flipped_x * scale_back), int(y * scale_back), int((flipped_x + w) * scale_back), int((y + h) * scale_back))

    return None


def draw_overlay(image, box, fps):
    center_x, center_y = FRAME_WIDTH // 2, FRAME_HEIGHT // 2
    cv2.circle(image, (center_x, center_y), 8, (0, 255, 0), 1)
    cv2.line(image, (0, center_y), (FRAME_WIDTH, center_y), (0, 255, 0), 1)
    cv2.line(image, (center_x, 0), (center_x, FRAME_HEIGHT), (0, 255, 0), 1)

    if box is not None:
        x1, y1, x2, y2 = box
        cv2.rectangle(image, (x1, y1), (x2, y2), (0, 0, 255), 2)

    cv2.putText(image, f"CAMERA-ONLY TEST | fps={fps:.1f}", (10, 25),
                cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)

    cv2.imshow("Camera-Only Test", image)
    return cv2.waitKey(1) & 0xFF


def main():
    print("Loading face detectors...")
    frontal_detector, profile_detector = load_face_detector()
    print("Face detectors loaded.")

    print(f"Connecting to camera at {CAMERA_HOST}...")
    camera_connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    camera_connection.connect((CAMERA_HOST, CAMERA_PORT))
    print("Connected to ESP32-S3-CAM. No motor connection will be attempted.")

    frame_counter = 0
    fps = 0.0
    fps_window_start = time.time()

    while True:
        try:
            jpeg_bytes = receive_one_frame(camera_connection)
        except ConnectionError:
            print("Camera connection lost - stopping.")
            break

        image = decode_frame(jpeg_bytes)
        if image is None:
            continue

        global _printed_frame_size
        if not _printed_frame_size:
            print(f"ACTUAL frame size received: {image.shape[1]}x{image.shape[0]} (width x height)")
            _printed_frame_size = True

        box = find_best_face_raw(frontal_detector, profile_detector, image)

        key_code = draw_overlay(image, box, fps)
        if key_code == ord('q'):
            print("Quit key pressed - stopping.")
            break

        frame_counter += 1
        now = time.time()
        if now - fps_window_start >= 1.0:
            fps = frame_counter / (now - fps_window_start)
            frame_counter = 0
            fps_window_start = now

    camera_connection.close()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()