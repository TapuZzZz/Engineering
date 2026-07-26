# =================================================================
#  PC SERVER - Part 2: Receive frames AND display them live
# =================================================================

import socket
import struct
import numpy as np    # NEW: lets us treat raw bytes as a numeric image array
import cv2             # NEW: OpenCV - decodes JPEG bytes and shows them in a window

# -----------------------------------------------------------------
#  Configuration - must match the ESP32 code exactly
# -----------------------------------------------------------------
LISTEN_IP   = "0.0.0.0"
LISTEN_PORT = 8000


# -----------------------------------------------------------------
#  Function: receive_exact
#  (unchanged from before)
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
#  (unchanged from before)
# -----------------------------------------------------------------
def receive_one_frame(connection):
    size_bytes = receive_exact(connection, 4)
    frame_size = struct.unpack("<I", size_bytes)[0]

    jpeg_bytes = receive_exact(connection, frame_size)

    return jpeg_bytes


# -----------------------------------------------------------------
#  Function: decode_and_show
#  What it does: takes raw JPEG bytes, turns them into an actual
#  image OpenCV can display, and shows it in a live window.
# -----------------------------------------------------------------
def decode_and_show(jpeg_bytes):
    # Step 1: treat the raw JPEG bytes as a 1D array of numbers (0-255 each)
    jpeg_array = np.frombuffer(jpeg_bytes, dtype=np.uint8)

    # Step 2: decode that array as an actual image (rows x columns x color channels)
    image = cv2.imdecode(jpeg_array, cv2.IMREAD_COLOR)

    if image is None:
        # Happens if a frame arrived corrupted/incomplete - skip it, don't crash
        print("Warning: failed to decode a frame, skipping.")
        return

    cv2.imshow("ESP32-CAM Live Feed", image)   # Opens/updates a window with this image

    cv2.waitKey(1)
    # IMPORTANT: OpenCV windows need this call to actually redraw and process
    # events (like being moved or closed). The "1" means "wait up to 1ms" -
    # we don't want to pause our loop, just give the window a chance to update.


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

    while True:
        try:
            jpeg_bytes = receive_one_frame(connection)
            frame_count += 1

            decode_and_show(jpeg_bytes)   # NEW: display instead of just printing size

            print(f"Received frame #{frame_count}, size in bytes: {len(jpeg_bytes)}")

        except ConnectionError:
            print("ESP32-CAM disconnected.")
            break

    connection.close()
    server_socket.close()
    cv2.destroyAllWindows()   # NEW: close the display window cleanly when we're done


if __name__ == "__main__":
    main()