#include "esp_camera.h"
#include <WiFi.h>
#include <ESPmDNS.h>

// ==========================================
// Settings
// ==========================================
#define SERIAL_BAUD        115200
#define CAM_XCLK_FREQ_HZ   20000000
#define CAM_JPEG_QUALITY   10
#define CAM_FB_COUNT       2
#define RESTART_DELAY_MS   5000

// ==========================================
// Network
// ==========================================
#define WIFI_TIMEOUT_MS    10000
#define MDNS_HOSTNAME      "esp-cam"

struct WifiNetwork {
  const char* ssid;
  const char* password;
};

const WifiNetwork WIFI_NETWORKS[] = {
  { "HomeNetwork",  "HomePassword"  },
  { "PhoneHotspot", "PhonePassword" },
};

const int WIFI_NETWORK_COUNT = sizeof(WIFI_NETWORKS) / sizeof(WIFI_NETWORKS[0]);

// ==========================================
// Camera Pins
// ==========================================
#define CAM_PIN_PWDN    -1
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK    15
#define CAM_PIN_SIOD     4
#define CAM_PIN_SIOC     5
#define CAM_PIN_D7      16
#define CAM_PIN_D6      17
#define CAM_PIN_D5      18
#define CAM_PIN_D4      12
#define CAM_PIN_D3      10
#define CAM_PIN_D2       8
#define CAM_PIN_D1       9
#define CAM_PIN_D0      11
#define CAM_PIN_VSYNC    6
#define CAM_PIN_HREF     7
#define CAM_PIN_PCLK    13

#define CAM_PIN_RGB     48

// ==========================================
// Main
// ==========================================
void restartWithError(const char* message) {
  Serial.println(message);
  showErrorLed();
  delay(RESTART_DELAY_MS);
  ESP.restart();
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  turnOffLed();

  if (!initCamera()) restartWithError("Error => Camera init failed");
  if (!applySensorSettings()) restartWithError("Error => Sensor init failed");

  initWiFi();
  if (!connectWiFi()) restartWithError("Error => WiFi connect failed");
  if (!startMdns()) restartWithError("Error => mDNS start failed");

  Serial.println("CAM ready => " MDNS_HOSTNAME ".local");
}

void loop() {
  if (!ensureNetwork()) return;
  captureFrame();
}