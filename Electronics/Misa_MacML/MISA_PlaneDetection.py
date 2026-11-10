import cv2
from ultralytics import YOLO

SOURCE = "http://MISA-CAM.local/stream"
MODEL_PATH = "yolo26n.pt"
TARGET_CLASS = "teddy bear"


def main():
    cap = cv2.VideoCapture(SOURCE)
    model = YOLO(MODEL_PATH)
    target_id = [i for i, n in model.names.items() if n == TARGET_CLASS][0]

    while True:
        ok, frame = cap.read()
        if not ok:
            break
        frame = cv2.rotate(frame, cv2.ROTATE_90_CLOCKWISE)
        results = model.track(frame, persist=True, tracker="bytetrack.yaml",
                              classes=[target_id], device="mps", verbose=False)
        for box in results[0].boxes:
            name = model.names[int(box.cls[0])]
            conf = float(box.conf[0])
            print(name, round(conf, 2), box.xyxy[0].tolist())
        cv2.imshow("cam", results[0].plot())
        if cv2.waitKey(1) == ord("q"):
            break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()