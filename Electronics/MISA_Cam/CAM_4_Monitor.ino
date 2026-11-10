void reportTemperature() {
  static unsigned long lastMs = 0;
  if (millis() - lastMs < TEMP_REPORT_MS) return;
  lastMs = millis();
  Serial.printf("Chip temp: %.1f C\n", temperatureRead());
}