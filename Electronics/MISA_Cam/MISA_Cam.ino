#include <WiFi.h>
#include <ESPmDNS.h>
#include "esp_camera.h"
#include "CAM_1_Settings.h"
#include "CAM_2_Configuration.h"
#include "CAM_3_SendingFrames.h"
#include "CAM_4_Monitor.h"

void setup() {
  Serial.begin(BAUD_RATE);
  delay(SERIAL_STARTUP_DELAY_MS);

  if (!initCameraConfig()) {
    Serial.println("=======================");
    Serial.println("[!] Config init failed!");
    Serial.println("=======================");

    while (true) delay(HALT_DELAY_MS);
  }

  if (!initWiFiConnection()) {
    Serial.println("=====================");
    Serial.println("[!] WiFi init failed!");
    Serial.println("=====================");

    while (true) delay(HALT_DELAY_MS);
  }
}


void loop() {
  camera_fb_t* frame = captureFrame();
  if (frame) {
    Serial.printf("[+] Frame size: %u bytes\n", frame->len);
    esp_camera_fb_return(frame);
  }
}