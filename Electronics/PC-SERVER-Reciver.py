# =================================================================
#  PC SERVER - Part 3: Receive frames, display them, and measure timing
# =================================================================

import socket
import struct
import time
import numpy as np
import cv2

# -----------------------------------------------------------------
#  Configuration - must match the ESP32 code exactly
# -----------------------------------------------------------------
LISTEN_IP   = "0.0.0.0"
LISTEN_PORT = 8000


# -----------------------------------------------------------------
#  Function: receive_exact
# -----------------------------------------------------------------
def receive_exact(connection, num_bytes):
    data = b""

    while len(data) < num_bytes:
        chunk = connection.recv(num_bytes - len(data))

        if not chunk:
            raise ConnectionError("Connection closed while receiving data.")

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
#  Function: decode_and_show
# -----------------------------------------------------------------
def decode_and_show(jpeg_bytes):
    jpeg_array = np.frombuffer(jpeg_bytes, dtype=np.uint8)
    image = cv2.imdecode(jpeg_array, cv2.IMREAD_COLOR)

    if image is None:
        print("Warning: failed to decode a frame, skipping.")
        return

    image = cv2.rotate(image, cv2.ROTATE_180)

    cv2.imshow("ESP32-CAM Live Feed", image)
    cv2.waitKey(1)


# =================================================================
#  MAIN PROGRAM
# =================================================================

def main():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((LISTEN_IP, LISTEN_PORT))
    server_socket.listen(1)

    print(f"Listening on port {LISTEN_PORT}, waiting for ESP32-CAM to connect...")

    connection, address = server_socket.accept()
    print(f"ESP32-CAM connected from {address}")

    frame_count = 0
    last_report_time = time.time()
    frames_since_report = 0

    while True:
        try:
            receive_start = time.time()
            jpeg_bytes = receive_one_frame(connection)
            receive_end = time.time()

            frame_count += 1
            frames_since_report += 1

            decode_start = time.time()
            decode_and_show(jpeg_bytes)
            decode_end = time.time()

            receive_ms = (receive_end - receive_start) * 1000
            decode_ms  = (decode_end - decode_start) * 1000

            print(f"Frame #{frame_count} | size: {len(jpeg_bytes)} bytes "
                  f"| receive: {receive_ms:.1f} ms | decode+show: {decode_ms:.1f} ms")

            now = time.time()
            if now - last_report_time >= 1.0:
                fps = frames_since_report / (now - last_report_time)
                print(f"---- Current FPS: {fps:.1f} ----")
                last_report_time = now
                frames_since_report = 0

        except ConnectionError:
            print("ESP32-CAM disconnected.")
            break

    connection.close()
    server_socket.close()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()