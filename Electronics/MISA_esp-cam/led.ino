void showErrorLed() {
  rgbLedWrite(CAM_PIN_RGB, 128, 0, 0);
}

void turnOffLed() {
  rgbLedWrite(CAM_PIN_RGB, 0, 0, 0);
}