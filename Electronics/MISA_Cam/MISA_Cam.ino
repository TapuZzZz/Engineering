#include <WiFi.h>
#include <ESPmDNS.h>
#include "esp_camera.h"


void setup() {
  Serial.begin(BAUD_RATE);
  delay(1000);

  if (!initCameraConfig()) {
    Serial.println("[!] Config init failed!");
    while (true) delay(1000);
  }

  if (!initWiFiConnection()) {
    Serial.println("[!] WiFi init failed!");
    while (true) delay(1000);
  }
}


void loop() {
  startStreamServer();
}   