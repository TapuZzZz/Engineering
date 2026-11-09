#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include "esp_camera.h"

const char* WIFI_SSID = "MISA-NET";
const char* WIFI_PASS = "03032007";
const char* HOST_NAME = "MISA-CAM";

const int UDP_PORT = 5005;
const int CHUNK_SIZE = 1400;
const unsigned long CLIENT_TIMEOUT_MS = 5000;

// פינים של Freenove ESP32-S3 CAM, לבדוק מול הלוח שלך
#define XCLK_GPIO  15
#define SIOD_GPIO  4
#define SIOC_GPIO  5
#define Y9_GPIO    16
#define Y8_GPIO    17
#define Y7_GPIO    18
#define Y6_GPIO    12
#define Y5_GPIO    10
#define Y4_GPIO    8
#define Y3_GPIO    9
#define Y2_GPIO    11
#define VSYNC_GPIO 6
#define HREF_GPIO  7
#define PCLK_GPIO  13

WiFiUDP udp;
IPAddress clientIp;
uint16_t clientPort = 0;
unsigned long lastHelloMs = 0;
uint16_t frameId = 0;

bool initCamera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO;
  config.pin_d1 = Y3_GPIO;
  config.pin_d2 = Y4_GPIO;
  config.pin_d3 = Y5_GPIO;
  config.pin_d4 = Y6_GPIO;
  config.pin_d5 = Y7_GPIO;
  config.pin_d6 = Y8_GPIO;
  config.pin_d7 = Y9_GPIO;
  config.pin_xclk = XCLK_GPIO;
  config.pin_pclk = PCLK_GPIO;
  config.pin_vsync = VSYNC_GPIO;
  config.pin_href = HREF_GPIO;
  config.pin_sccb_sda = SIOD_GPIO;
  config.pin_sccb_scl = SIOC_GPIO;
  config.pin_pwdn = -1;
  config.pin_reset = -1;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 15;
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera error: 0x%x (%s)\n", err, esp_err_to_name(err));
  }
  return err == ESP_OK;
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOST_NAME);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  WiFi.setSleep(false);
  Serial.println("\nWiFi connected");
}

void startMdns() {
  if (MDNS.begin(HOST_NAME)) {
    Serial.println("Name: MISA-CAM.local");
  }
}

void checkForHello() {
  int size = udp.parsePacket();
  if (size > 0) {
    clientIp = udp.remoteIP();
    clientPort = udp.remotePort();
    lastHelloMs = millis();
    udp.flush();
  }
}

bool hasClient() {
  return clientPort != 0 && (millis() - lastHelloMs) < CLIENT_TIMEOUT_MS;
}

void sendChunk(const uint8_t* data, size_t len, uint8_t index, uint8_t total) {
  udp.beginPacket(clientIp, clientPort);
  udp.write((uint8_t)(frameId >> 8));
  udp.write((uint8_t)(frameId & 0xFF));
  udp.write(index);
  udp.write(total);
  udp.write(data, len);
  udp.endPacket();
}

void sendFrame(camera_fb_t* fb) {
  uint8_t total = (fb->len + CHUNK_SIZE - 1) / CHUNK_SIZE;
  for (uint8_t i = 0; i < total; i++) {
    size_t offset = (size_t)i * CHUNK_SIZE;
    size_t len = min((size_t)CHUNK_SIZE, fb->len - offset);
    sendChunk(fb->buf + offset, len, i, total);
    delay(1);  // לא להציף את ה-buffer של ה-Wi-Fi
  }
  frameId++;
}

void streamOneFrame() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) return;
  sendFrame(fb);
  esp_camera_fb_return(fb);
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // זמן ל-Serial Monitor להיפתח
  if (!initCamera()) {
    Serial.println("Camera init failed");
    while (true) delay(1000);
  }
  connectWifi();
  startMdns();
  udp.begin(UDP_PORT);
}

void loop() {
  checkForHello();
  if (hasClient()) {
    streamOneFrame();
  }
}