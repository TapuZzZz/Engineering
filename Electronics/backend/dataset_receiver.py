"""
Or Eithan - Dataset Receiver
==============================
This is the ONLY thing running on the PC during data collection.
The ESP32-CAM hosts its own dashboard (in C++) with a "Capture" button.
Every time you press it, the ESP32 uploads the current frame here, and
this script just saves it to disk as a numbered JPEG.
 
Once you've collected enough images (with the plane, without it, at
different distances/angles/lighting), zip the dataset/ folder and upload
it to Edge Impulse to train the FOMO model.
 
Run with:
    python dataset_receiver.py
 
Leaves it listening on port 5001 (matches PC_UPLOAD_PORT in the .ino).
"""
 
from flask import Flask, request
from pathlib import Path
from datetime import datetime
 
DATASET_DIR = Path(__file__).parent / "dataset"
DATASET_DIR.mkdir(exist_ok=True)
 
app = Flask(__name__)
 
 
@app.route("/upload", methods=["POST"])
def upload():
    image_bytes = request.get_data()
    if not image_bytes:
        return {"ok": False, "error": "empty body"}, 400
 
    try:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
        filename = DATASET_DIR / f"img_{timestamp}.jpg"
        
        # כתיבת הקובץ
        filename.write_bytes(image_bytes)
     
        total = len(list(DATASET_DIR.glob("*.jpg")))
        print(f"Saved {filename.name}  (total in dataset: {total})")
     
        return {"ok": True, "filename": filename.name, "total": total}
        
    except Exception as e:
        print(f"Error saving image: {e}")
        return {"ok": False, "error": "failed to save image"}, 500
        
if __name__ == "__main__":
    print(f"Dataset receiver ready. Saving images to: {DATASET_DIR}")
    print("Waiting for captures from the ESP32-CAM dashboard...")
    app.run(host="0.0.0.0", port=5001, debug=False)
 