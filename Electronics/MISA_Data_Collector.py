# =================================================================
#  MISA DATA COLLECTOR - captures labeled training images from the
#  live ESP32-S3-CAM feed, for later use training a custom
#  detection/classification model (fighter vs. airliner vs. empty).
#
#  Controls:
#    Hold 'f'  -> continuously save frames to dataset/fighter/
#    Hold 'a'  -> continuously save frames to dataset/airliner/
#    Hold 'e'  -> continuously save frames to dataset/empty/
#    'q'       -> quit
#
#  Images are saved through the SAME decode+rotate pipeline used by
#  the live tracking system, so the training data matches exactly
#  what the model will see at inference time later.
# =================================================================

import socket
import struct
import os
import time
from datetime import datetime

import numpy as np
import cv2

# -----------------------------------------------------------------
#  Configuration
# -----------------------------------------------------------------
CAMERA_HOST = "esp32cam.local"
CAMERA_PORT = 8000

DATASET_ROOT = "dataset"
CLASS_FOLDERS = {
    ord('f'): "fighter",
    ord('a'): "airliner",
    ord('e'): "empty",
}

CAPTURE_INTERVAL_SECONDS = 0.15   # While a class key is held, save at most one image this often -
                                    # fast enough to gather many images quickly, slow enough that
                                    # consecutive frames still differ a little (slight motion/angle)


# -----------------------------------------------------------------
#  Function: ensure_dataset_folders_exist
#  What it does: creates dataset/fighter, dataset/airliner, and
#  dataset/empty if they don't already exist, so saving never fails
#  because of a missing folder.
# -----------------------------------------------------------------
def ensure_dataset_folders_exist():
    for class_name in CLASS_FOLDERS.values():
        folder_path = os.path.join(DATASET_ROOT, class_name)
        os.makedirs(folder_path, exist_ok=True)


# -----------------------------------------------------------------
#  Function: count_existing_images
#  What it does: counts how many images already exist in each class
#  folder, so the on-screen counters start accurate even if you stop
#  and resume collection across multiple sessions.
# -----------------------------------------------------------------
def count_existing_images():
    counts = {}
    for class_name in CLASS_FOLDERS.values():
        folder_path = os.path.join(DATASET_ROOT, class_name)
        counts[class_name] = len([
            f for f in os.listdir(folder_path)
            if f.lower().endswith((".jpg", ".jpeg"))
        ])
    return counts


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
#  What it does: same decode + rotation used by the live tracking
#  system - critical that training images match inference images.
# -----------------------------------------------------------------
def decode_frame(jpeg_bytes):
    jpeg_array = np.frombuffer(jpeg_bytes, dtype=np.uint8)
    image = cv2.imdecode(jpeg_array, cv2.IMREAD_COLOR)

    if image is None:
        return None

    image = cv2.rotate(image, cv2.ROTATE_180)
    return image


# -----------------------------------------------------------------
#  Function: save_labeled_image
#  What it does: saves the given frame into the correct class
#  folder, with a filename that includes a timestamp down to the
#  millisecond - guarantees unique filenames even when capturing
#  quickly in continuous mode.
# -----------------------------------------------------------------
def save_labeled_image(image, class_name):
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    filename = f"{class_name}_{timestamp}.jpg"
    filepath = os.path.join(DATASET_ROOT, class_name, filename)
    cv2.imwrite(filepath, image)
    return filename


# -----------------------------------------------------------------
#  Function: draw_overlay
#  What it does: shows live counts per class and instructions,
#  so you always know exactly how much data you've collected
#  without checking the file explorer.
# -----------------------------------------------------------------
def draw_overlay(image, counts, currently_saving_class):
    display_image = image.copy()

    y_position = 25
    cv2.putText(display_image, "Data Collector - MISA", (10, y_position),
                cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)

    y_position += 25
    for class_name in ["fighter", "airliner", "empty"]:
        color = (0, 255, 0) if currently_saving_class == class_name else (255, 255, 255)
        text = f"{class_name}: {counts[class_name]}"
        cv2.putText(display_image, text, (10, y_position),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.55, color, 2)
        y_position += 22

    y_position += 5
    cv2.putText(display_image, "HOLD: f=fighter  a=airliner  e=empty  |  q=quit",
                (10, image.shape[0] - 15), cv2.FONT_HERSHEY_SIMPLEX, 0.45, (0, 200, 255), 1)

    cv2.imshow("MISA Data Collector", display_image)


# =================================================================
#  MAIN PROGRAM
# =================================================================

def main():
    ensure_dataset_folders_exist()
    counts = count_existing_images()

    print("Starting counts:", counts)
    print(f"Connecting to camera at {CAMERA_HOST}...")

    camera_connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    camera_connection.connect((CAMERA_HOST, CAMERA_PORT))
    print("Connected to ESP32-S3-CAM.")

    last_capture_time = 0.0

    while True:
        try:
            jpeg_bytes = receive_one_frame(camera_connection)
        except ConnectionError:
            print("Camera connection lost - stopping.")
            break

        image = decode_frame(jpeg_bytes)

        if image is None:
            continue

        key_code = cv2.waitKey(1) & 0xFF

        currently_saving_class = None

        if key_code == ord('q'):
            print("Quit requested.")
            break

        if key_code in CLASS_FOLDERS:
            class_name = CLASS_FOLDERS[key_code]
            now = time.time()

            # Rate-limit saving so continuous key-hold doesn't flood the disk
            # with near-identical frames - CAPTURE_INTERVAL_SECONDS apart is
            # enough time for slight natural movement/angle change.
            if now - last_capture_time >= CAPTURE_INTERVAL_SECONDS:
                save_labeled_image(image, class_name)
                counts[class_name] += 1
                last_capture_time = now

            currently_saving_class = class_name

        draw_overlay(image, counts, currently_saving_class)

    camera_connection.close()
    cv2.destroyAllWindows()

    print("Final counts:", counts)


if __name__ == "__main__":
    main()