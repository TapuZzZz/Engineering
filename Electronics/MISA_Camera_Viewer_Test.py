# =================================================================
#  MISA CAMERA VIEWER - ISOLATED TEST
#  Does ONE thing: connects to the camera and shows the live feed.
#  No face detection, no motor commands, no dashboard. Pure
#  connectivity + image check.
# =================================================================

import socket
import struct
import time

import numpy as np
import cv2

CAMERA_HOST = "esp32cam.local"
CAMERA_PORT = 8000


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
    jpeg_bytes = receive_exact(connection, frame_size)
    return jpeg_bytes


def decode_frame(jpeg_bytes):
    jpeg_array = np.frombuffer(jpeg_bytes, dtype=np.uint8)
    image = cv2.imdecode(jpeg_array, cv2.IMREAD_COLOR)
    return image


def main():
    print(f"Connecting to camera at {CAMERA_HOST}:{CAMERA_PORT}...")
    camera_connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    camera_connection.connect((CAMERA_HOST, CAMERA_PORT))
    print("Connected. Press 'q' in the video window to quit.")

    frame_count = 0
    last_report_time = time.time()
    frames_since_report = 0

    while True:
        try:
            jpeg_bytes = receive_one_frame(camera_connection)
        except ConnectionError:
            print("Camera connection lost - stopping.")
            break

        image = decode_frame(jpeg_bytes)

        if image is None:
            print("Warning: failed to decode a frame, skipping.")
            continue

        frame_count += 1
        frames_since_report += 1

        cv2.putText(image, f"Frame #{frame_count}", (10, 25),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)

        cv2.imshow("MISA Camera Viewer - raw feed, no rotation applied", image)

        now = time.time()
        if now - last_report_time >= 1.0:
            fps = frames_since_report / (now - last_report_time)
            print(f"FPS: {fps:.1f} | frame size: {len(jpeg_bytes)} bytes")
            last_report_time = now
            frames_since_report = 0

        key = cv2.waitKey(1) & 0xFF
        if key == ord('q'):
            print("Quit requested.")
            break

    camera_connection.close()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()