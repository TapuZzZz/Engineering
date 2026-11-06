#include "esp_camera.h"
#include "esp_http_server.h"
#include <WiFi.h>
#include <ESPmDNS.h>

void setup() {
  serialSetup();
  cameraSetup();
  networkSetup();
  mdnsSetup();
  webServerSetup();
  Serial.println("CAM ready");
}

void loop() {
  networkMonitor();
  delay(10);
}