void captureFrame() {
  camera_fb_t* frame = esp_camera_fb_get();
  if (frame == nullptr) {
    Serial.println("Error => captureFrame() >> Capture failed");
    showErrorLed();
    return;
  }
  turnOffLed();
  esp_camera_fb_return(frame);
}