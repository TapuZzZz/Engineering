import cv2
from ultralytics import YOLO

SOURCE = 0
MODEL_PATH = "yolo26n.pt"


def main():
    cap = cv2.VideoCapture(SOURCE)
    model = YOLO(MODEL_PATH)

    while True:
        ok, frame = cap.read()
        if not ok:
            break
        results = model.track(frame, persist=True, tracker="bytetrack.yaml",
                              device="mps", verbose=False)
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