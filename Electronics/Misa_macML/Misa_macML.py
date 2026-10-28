import os
import socket
import threading
import time

import cv2
from ultralytics import YOLO

# ===================== Settings =====================
DISCOVERY_PORT = 5005                  # ESPs announce themselves here
DEVICES        = ("CAM", "ACTIVATOR")  # wait for all of these
TARGET_PORT    = 4210                  # ACTIVATOR listens here

TARGET_NAME    = "bottle"              # also: "cup", "cell phone", "remote", "book"
MIN_CONFIDENCE = 0.25
IMAGE_SIZE     = 480                   # matches the camera's 480x320
PT_PATH        = "yolo11n.pt"
COREML_PATH    = f"yolo11n_{IMAGE_SIZE}.mlpackage"  # created on first run

WINDOW = "Tracker"

# ===================== Axis mapping =====================
SWAP_AXES = False   # True if horizontal movement moves the TILT servo
INVERT_X  = False   # True if PAN moves away from the target
INVERT_Y  = True    # TILT was moving away from the target


# ===================== Discovery =====================
def open_discovery_socket():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("", DISCOVERY_PORT))
    return sock


def parse_hello(data):
    """'HELLO,CAM' -> 'CAM', anything else -> None."""
    text = data.decode(errors="ignore").strip()
    if not text.startswith("HELLO,"):
        return None
    return text.split(",", 1)[1]


def discover_devices():
    """Waits until every device announced itself. Returns {name: ip}."""
    sock = open_discovery_socket()
    found = {}
    while len(found) < len(DEVICES):
        data, (ip, _) = sock.recvfrom(64)
        name = parse_hello(data)
        if name in DEVICES and name not in found:
            found[name] = ip
            print(f"  {name} connected: {ip}")
    sock.close()
    return found


# ===================== Stream reader =====================
class LatestFrameReader:
    """Reads the stream in the background and keeps only the newest frame."""

    def __init__(self, url):
        self.capture = cv2.VideoCapture(url, cv2.CAP_FFMPEG, [
            cv2.CAP_PROP_OPEN_TIMEOUT_MSEC, 5000,
            cv2.CAP_PROP_READ_TIMEOUT_MSEC, 5000,
        ])
        if not self.capture.isOpened():
            raise RuntimeError(f"Cannot open stream: {url}")
        self.frame = None
        self.lock = threading.Lock()
        threading.Thread(target=self._read_loop, daemon=True).start()

    def _read_loop(self):
        while True:
            ok, frame = self.capture.read()
            if not ok:
                time.sleep(0.01)
                continue
            with self.lock:
                self.frame = frame

    def get(self):
        """Returns the newest frame once, or None if no new frame arrived."""
        with self.lock:
            frame, self.frame = self.frame, None
        return frame


# ===================== Model =====================
def load_model():
    """CoreML (fastest on Mac) if possible, otherwise PyTorch on CPU."""
    try:
        if not os.path.exists(COREML_PATH):
            exported = YOLO(PT_PATH).export(format="coreml", imgsz=IMAGE_SIZE)
            os.rename(exported, COREML_PATH)
        return YOLO(COREML_PATH, task="detect"), None
    except Exception as error:
        print(f"CoreML not available ({error}) - using CPU")
        return YOLO(PT_PATH), "cpu"


def class_id(model, name):
    """Returns the model's class number for a class name."""
    for cid, cname in model.names.items():
        if cname == name:
            return cid
    raise ValueError(f"Unknown class: {name}")


# ===================== Detection =====================
class Detector:
    """Finds and tracks one object class."""

    def __init__(self):
        self.model, self.device = load_model()
        self.target_id = class_id(self.model, TARGET_NAME)
        self.last_id = None

    def find(self, frame):
        """Returns (x1, y1, x2, y2) of the tracked target, or None."""
        results = self.model.track(frame, persist=True, tracker="bytetrack.yaml",
                                   classes=[self.target_id], conf=MIN_CONFIDENCE,
                                   imgsz=IMAGE_SIZE, device=self.device, verbose=False)
        boxes = results[0].boxes
        if len(boxes) == 0:
            return None
        return self.pick_box(boxes)

    def pick_box(self, boxes):
        """Keeps following the same object (track id) when possible."""
        index = int(boxes.conf.argmax())
        if boxes.id is not None:
            ids = boxes.id.int().tolist()
            if self.last_id in ids:
                index = ids.index(self.last_id)
            self.last_id = ids[index]
        return boxes.xyxy[index].int().tolist()


def box_offset(box, frame):
    """Target offset from frame center, in percent (-100..100)."""
    x1, y1, x2, y2 = box
    h, w = frame.shape[:2]
    dx = ((x1 + x2) / 2 - w / 2) / (w / 2) * 100
    dy = ((y1 + y2) / 2 - h / 2) / (h / 2) * 100
    return int(dx), int(dy)


def to_servo_offset(offset):
    """Maps image offset to (pan, tilt) offset by how the camera is mounted."""
    dx, dy = offset
    if SWAP_AXES:
        dx, dy = dy, dx
    if INVERT_X:
        dx = -dx
    if INVERT_Y:
        dy = -dy
    return dx, dy


# ===================== Messaging =====================
def open_sender(ip):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    return sock, (ip, TARGET_PORT)


def send_message(sender, text):
    sock, address = sender
    sock.sendto(text.encode(), address)


# ===================== Drawing =====================
def draw_target(frame, box, offset):
    x1, y1, x2, y2 = box
    cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 0, 255), 2)
    cv2.putText(frame, f"dx={offset[0]} dy={offset[1]}", (x1, y1 - 8),
                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 1)


def draw_overlay(frame, fps):
    h, w = frame.shape[:2]
    cv2.drawMarker(frame, (w // 2, h // 2), (255, 255, 0), cv2.MARKER_CROSS, 20, 1)
    cv2.putText(frame, f"FPS {fps:.0f}", (8, 20),
                cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)


def measure_fps(last_time):
    now = time.time()
    return now, 1 / max(now - last_time, 1e-6)


# ===================== Window + laser calibration click =====================
def on_click(event, x, y, flags, view):
    """Click the laser dot to print its offset (for parallax calibration)."""
    if event != cv2.EVENT_LBUTTONDOWN:
        return
    h, w = view["size"]
    dx = int((x - w / 2) / (w / 2) * 100)
    dy = int((y - h / 2) / (h / 2) * 100)
    pan, tilt = to_servo_offset((dx, dy))
    print(f"Laser dot: X={pan}  Y={tilt}   (+ the DIST shown on the TFT)")


def open_window():
    view = {"size": (480, 320)}
    cv2.namedWindow(WINDOW)
    cv2.setMouseCallback(WINDOW, on_click, view)
    return view


def show_frame(frame, view, last_time):
    last_time, fps = measure_fps(last_time)
    draw_overlay(frame, fps)
    view["size"] = frame.shape[:2]
    cv2.imshow(WINDOW, frame)
    return last_time


# ===================== Main =====================
def process_frame(detector, sender, frame):
    frame = cv2.rotate(frame, cv2.ROTATE_90_CLOCKWISE)
    box = detector.find(frame)
    if box is None:
        send_message(sender, "N")
        return frame
    offset = box_offset(box, frame)
    pan, tilt = to_servo_offset(offset)
    send_message(sender, f"T,{pan},{tilt}")
    draw_target(frame, box, offset)
    return frame


def run_loop(detector, reader, sender):
    view = open_window()
    last_time = time.time()
    while cv2.waitKey(1) != ord("q"):
        frame = reader.get()
        if frame is None:
            continue
        frame = process_frame(detector, sender, frame)
        last_time = show_frame(frame, view, last_time)


def main():
    detector = Detector()
    print("Waiting for devices...")
    devices = discover_devices()
    reader = LatestFrameReader(f"http://{devices['CAM']}/stream")
    sender = open_sender(devices["ACTIVATOR"])
    print("All connected - tracking started")
    run_loop(detector, reader, sender)
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()