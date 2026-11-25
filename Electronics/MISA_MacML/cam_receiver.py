"""
MISA CAM - test receiver for the computer.

Run this BEFORE powering the camera. It:
  1. waits for the camera's HELLO and answers START,
  2. glues the incoming pieces back into pictures and shows them,
  3. sends PING every second, so the camera knows we are still here.

Install once:   pip install opencv-python numpy
Run:            python3 cam_receiver.py
Quit:           press  q  in the video window
"""

import socket
import time

import cv2
import numpy as np


# ---------- Settings (must match CAM_1_Settings.h) ----------
PC_UDP_PORT = 5050
PING_INTERVAL_SECONDS = 1.0
SOCKET_TIMEOUT_SECONDS = 0.1
RECEIVE_BUFFER_BYTES = 2048
SOCKET_BUFFER_BYTES = 1024 * 1024
WINDOW_NAME = "MISA CAM"

# The camera sits sideways on the board, so every picture is turned upright here.
# If the picture comes out upside down, change to cv2.ROTATE_90_COUNTERCLOCKWISE.
FRAME_ROTATION = cv2.ROTATE_90_CLOCKWISE

# ---------- Message types (must match CAM_6_MacLink.h) ----------
MSG_HELLO = ord("H")
MSG_START = ord("S")
MSG_PING = ord("P")
MSG_FRAME_CHUNK = ord("F")
CHUNK_HEADER_SIZE = 4


class FrameAssembler:
    """Collects the pieces of one picture until the picture is complete."""

    def __init__(self):
        self.frame_id = None
        self.chunk_count = 0
        self.chunks = {}
        self.finished_frame_id = None
        self.lost_frames = 0

    def add_chunk(self, frame_id, chunk_index, chunk_count, data):
        """Adds one piece. Returns the full JPEG when the picture is complete, otherwise None."""
        if frame_id == self.finished_frame_id:
            return None  # a late piece of a picture we already showed

        if frame_id != self.frame_id:
            self._start_new_frame(frame_id, chunk_count)

        self.chunks[chunk_index] = data

        if len(self.chunks) < self.chunk_count:
            return None

        return self._finish_frame()

    def _start_new_frame(self, frame_id, chunk_count):
        if self.chunks:
            self.lost_frames += 1  # the previous picture never got all its pieces
        self.frame_id = frame_id
        self.chunk_count = chunk_count
        self.chunks = {}

    def _finish_frame(self):
        jpeg = b"".join(self.chunks[index] for index in range(self.chunk_count))
        self.finished_frame_id = self.frame_id
        self.frame_id = None
        self.chunks = {}
        return jpeg


class FpsCounter:
    """Counts how many pictures arrive per second."""

    def __init__(self):
        self.value = 0.0
        self._frames = 0
        self._start_time = time.time()

    def tick(self):
        self._frames += 1
        elapsed = time.time() - self._start_time
        if elapsed >= 1.0:
            self.value = self._frames / elapsed
            self._frames = 0
            self._start_time = time.time()


def open_socket():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, SOCKET_BUFFER_BYTES)
    sock.bind(("", PC_UDP_PORT))
    sock.settimeout(SOCKET_TIMEOUT_SECONDS)
    return sock


def receive_packet(sock):
    """Returns (packet, sender address), or (None, None) if nothing arrived."""
    try:
        return sock.recvfrom(RECEIVE_BUFFER_BYTES)
    except socket.timeout:
        return None, None


def decode_jpeg(jpeg):
    """Turns JPEG bytes into an image. Returns None if the picture is broken."""
    return cv2.imdecode(np.frombuffer(jpeg, dtype=np.uint8), cv2.IMREAD_COLOR)


def rotate_upright(image):
    return cv2.rotate(image, FRAME_ROTATION)


def show_frame(jpeg, fps, lost_frames):
    image = decode_jpeg(jpeg)
    if image is None:
        return  # a broken picture - skip it

    image = rotate_upright(image)

    status = f"{fps:.1f} fps | lost: {lost_frames}"
    cv2.putText(image, status, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
    cv2.imshow(WINDOW_NAME, image)


def is_quit_pressed():
    return cv2.waitKey(1) & 0xFF == ord("q")


def main():
    sock = open_socket()
    assembler = FrameAssembler()
    fps = FpsCounter()
    camera_address = None
    last_ping_time = 0.0

    print(f"Waiting for MISA CAM on port {PC_UDP_PORT}...")

    while True:
        packet, sender = receive_packet(sock)

        if packet is None:
            if is_quit_pressed():
                break

        elif packet[0] == MSG_HELLO:
            camera_address = sender
            sock.sendto(bytes([MSG_START]), camera_address)
            print(f"Camera found at {sender[0]} - sent START")

        elif packet[0] == MSG_FRAME_CHUNK and len(packet) > CHUNK_HEADER_SIZE:
            frame_id, chunk_index, chunk_count = packet[1], packet[2], packet[3]
            jpeg = assembler.add_chunk(frame_id, chunk_index, chunk_count,
                                       packet[CHUNK_HEADER_SIZE:])
            if jpeg is not None:
                fps.tick()
                show_frame(jpeg, fps.value, assembler.lost_frames)
                if is_quit_pressed():
                    break

        if camera_address and time.time() - last_ping_time >= PING_INTERVAL_SECONDS:
            sock.sendto(bytes([MSG_PING]), camera_address)
            last_ping_time = time.time()

    sock.close()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()