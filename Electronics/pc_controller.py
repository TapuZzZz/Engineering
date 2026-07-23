"""
PC Controller
-------------
מתחבר לשני מכשירים:
  1. ESP32-CAM  (TCP סטרימינג פריימים - ipCamHost:ipCamPort) - כמו receive_stream.py
  2. ESP32 בקר לייזר  (TCP פקודות ON/OFF - ipCtrlHost:ipCtrlPort)

לוגיקה:
  - כל עוד המצלמה מחוברת ושולחת פריימים -> מציגים וידאו לייב.
  - "ירי" מתבצע רק כשאתה מחזיק את מקש הרווח (Hold-to-fire).
    כל עוד המקש לחוץ, נשלח "ON\n" לבקר בקצב גבוה (heartbeat).
    ברגע ששחררת את המקש (או שנסגר החלון) - נשלח "OFF\n" מיידית.
  - אם החיבור לבקר נופל, אין שליחה - וה-watchdog בצד ESP32 יכבה
    את עצמו תוך 400ms ממילא.

הרצה:
  python pc_controller.py --cam-ip 192.168.0.176 --cam-port 3333 \
                           --ctrl-ip 192.168.0.XXX --ctrl-port 4444
"""

import argparse
import socket
import struct
import threading
import time

import cv2
import numpy as np


class LaserController:
    def __init__(self, ip, port):
        self.ip = ip
        self.port = port
        self.sock = None
        self.connected = False
        self.lock = threading.Lock()
        self._connect()

    def _connect(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(2)
            self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            self.sock.connect((self.ip, self.port))
            self.connected = True
            print(f"[CTRL] Connected to laser controller at {self.ip}:{self.port}")
        except OSError as e:
            print(f"[CTRL] Connection failed: {e}")
            self.connected = False

    def send(self, cmd: str):
        with self.lock:
            if not self.connected:
                self._connect()
                if not self.connected:
                    return
            try:
                self.sock.sendall((cmd + "\n").encode())
            except OSError as e:
                print(f"[CTRL] Send failed ({e}), will reconnect")
                self.connected = False

    def close(self):
        with self.lock:
            if self.sock:
                try:
                    self.sock.sendall(b"OFF\n")
                except OSError:
                    pass
                self.sock.close()


def heartbeat_loop(controller: LaserController, fire_flag: threading.Event, stop_flag: threading.Event):
    """שולח ON כל עוד fire_flag דלוק, ו-OFF ברגע שהוא כבה."""
    was_firing = False
    while not stop_flag.is_set():
        if fire_flag.is_set():
            controller.send("ON")
            was_firing = True
            # heartbeat כל 60ms -> כ-20 פקודות בתוך חלון ה-timeout של 1200ms
            # בצד ESP32, כך שגם אם כמה פקודות בודדות הולכות לאיבוד/מתעכבות
            # ברשת, עדיין יגיע מספיק heartbeat כדי לשמור על הלייזר דלוק.
            time.sleep(0.06)
        else:
            if was_firing:
                controller.send("OFF")
                was_firing = False
            time.sleep(0.05)


def recv_exact(sock, n):
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            return None
        buf += chunk
    return buf


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cam-ip", required=True)
    parser.add_argument("--cam-port", type=int, required=True)
    parser.add_argument("--ctrl-ip", required=True)
    parser.add_argument("--ctrl-port", type=int, required=True)
    args = parser.parse_args()

    # --- חיבור לבקר הלייזר ---
    controller = LaserController(args.ctrl_ip, args.ctrl_port)
    fire_flag = threading.Event()
    stop_flag = threading.Event()
    hb_thread = threading.Thread(target=heartbeat_loop, args=(controller, fire_flag, stop_flag), daemon=True)
    hb_thread.start()

    # --- חיבור למצלמה ---
    cam_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    cam_sock.connect((args.cam_ip, args.cam_port))
    print(f"[CAM] Connected to ESP32-CAM at {args.cam_ip}:{args.cam_port}")

    print("\nהוראות: לחץ SPACE פעם אחת כדי לירות/לחמש, ולחץ שוב כדי לבטל (טוגל). ESC ליציאה.\n")

    space_down_prev = False

    try:
        while True:
            # קריאת גודל פריים (4 בייטים, כמו בפרוטוקול הקיים שלך)
            size_data = recv_exact(cam_sock, 4)
            if size_data is None:
                print("[CAM] Connection closed")
                break
            frame_size = struct.unpack("<I", size_data)[0]

            jpg_data = recv_exact(cam_sock, frame_size)
            if jpg_data is None:
                break

            frame = cv2.imdecode(np.frombuffer(jpg_data, dtype=np.uint8), cv2.IMREAD_COLOR)
            if frame is None:
                continue

            frame = cv2.rotate(frame, cv2.ROTATE_180)

            # --- קרוסהייר: שני קווים דקים כחולים דרך מרכז הפריים ---
            h, w = frame.shape[:2]
            cx, cy = w // 2, h // 2
            cv2.line(frame, (0, cy), (w, cy), (255, 0, 0), 1)  # אופקי
            cv2.line(frame, (cx, 0), (cx, h), (255, 0, 0), 1)  # אנכי

            status_text = "ARMED - FIRING" if fire_flag.is_set() else "idle (press SPACE to arm)"
            color = (0, 0, 255) if fire_flag.is_set() else (0, 255, 0)
            cv2.putText(frame, f"Status: {status_text}", (10, 30),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.8, color, 2)
            cv2.imshow("PC Controller", frame)

            key = cv2.waitKey(1) & 0xFF
            if key == 27:  # ESC
                break

            # --- טוגל: לחיצה אחת = הדלקה, לחיצה נוספת = כיבוי ---
            # (edge-triggered - מגיב רק למעבר "לא לחוץ -> לחוץ", לא לכל
            # החזקה, כדי שלחיצה ארוכה אחת לא תיספר כמה פעמים)
            space_now = (key == ord(' '))
            if space_now and not space_down_prev:
                if fire_flag.is_set():
                    fire_flag.clear()
                    print("[TOGGLE] Disarmed")
                else:
                    fire_flag.set()
                    print("[TOGGLE] Armed / Firing")
            space_down_prev = space_now

    finally:
        fire_flag.clear()
        stop_flag.set()
        time.sleep(0.2)
        controller.close()
        cam_sock.close()
        cv2.destroyAllWindows()
        print("Shut down cleanly.")


if __name__ == "__main__":
    main()
