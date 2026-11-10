#include <WiFi.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>
#include "esp_camera.h"
#include "esp_http_server.h"

void setup() {
  Serial.begin(115200);
  delay(1000);
  if (!initCamera()) {
    Serial.println("Camera init failed");
    while (true) delay(1000);
  }
  connectWifi();
  WiFi.setSleep(false);
  startMdns();
  startStreamServer();
  Serial.println(WiFi.localIP());
}

void loop() {
  reportTemperature();
  delay(100);
}   