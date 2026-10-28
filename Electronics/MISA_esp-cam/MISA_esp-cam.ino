#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// ---------- Freenove ESP32-S3 WROOM CAM pin-out ----------
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

// ---------- WiFi ----------
const char* WIFI_SSID = "Amal-Student";
const char* WIFI_PASS = "Amal1@st";

WebServer server(80);

// ---------- Last image info ----------
int imageCount  = 0;
int imageWidth  = 0;
int imageHeight = 0;
int imageSize   = 0;

// ---------- Web page ----------
const char PAGE[] = R"(
<!DOCTYPE html>
<html>
<body>
  <h1>ESP32 Camera</h1>
  <img id="photo" src="/capture">
  <p id="info"></p>

  <script>
    const photo = document.getElementById("photo");
    const info  = document.getElementById("info");

    function loadInfo() {
      fetch("/info")
        .then(response => response.text())
        .then(text => info.textContent = text);
    }

    function loadPhoto() {
      photo.src = "/capture?t=" + Date.now();
    }

    photo.onload = loadInfo;
    setInterval(loadPhoto, 3000);
  </script>
</body>
</html>
)";

// ---------- Camera ----------
camera_config_t buildCameraConfig() {
  camera_config_t config = {};

  config.pin_pwdn     = CAM_PIN_PWDN;
  config.pin_reset    = CAM_PIN_RESET;
  config.pin_xclk     = CAM_PIN_XCLK;
  config.pin_sccb_sda = CAM_PIN_SIOD;
  config.pin_sccb_scl = CAM_PIN_SIOC;

  config.pin_d7    = CAM_PIN_D7;
  config.pin_d6    = CAM_PIN_D6;
  config.pin_d5    = CAM_PIN_D5;
  config.pin_d4    = CAM_PIN_D4;
  config.pin_d3    = CAM_PIN_D3;
  config.pin_d2    = CAM_PIN_D2;
  config.pin_d1    = CAM_PIN_D1;
  config.pin_d0    = CAM_PIN_D0;
  config.pin_vsync = CAM_PIN_VSYNC;
  config.pin_href  = CAM_PIN_HREF;
  config.pin_pclk  = CAM_PIN_PCLK;

  config.xclk_freq_hz = 20000000;
  config.ledc_timer   = LEDC_TIMER_0;
  config.ledc_channel = LEDC_CHANNEL_0;

  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count     = 1;
  config.fb_location  = CAMERA_FB_IN_DRAM;
  config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;

  return config;
}

esp_err_t cameraInit() {
  camera_config_t config = buildCameraConfig();
  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK) {
    Serial.printf("Camera init failed, error 0x%x\n", err);
    while (true) { delay(1000); }
  }

  return err;
}

// ---------- WiFi ----------
void wifiConnect() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Open: http://");
  Serial.println(WiFi.localIP());
}

// ---------- Handlers ----------
void saveImageInfo(camera_fb_t* fb) {
  imageCount++;
  imageWidth  = fb->width;
  imageHeight = fb->height;
  imageSize   = fb->len;
}

void handleRoot() {
  server.send(200, "text/html", PAGE);
}

void handleCapture() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (fb == nullptr) {
    server.send(500, "text/plain", "Capture failed");
    return;
  }

  saveImageInfo(fb);

  server.setContentLength(fb->len);
  server.send(200, "image/jpeg", "");
  server.client().write(fb->buf, fb->len);

  esp_camera_fb_return(fb);
}

void handleInfo() {
  String text = "Photo #";
  text += imageCount;
  text += " | ";
  text += imageWidth;
  text += "x";
  text += imageHeight;
  text += " | ";
  text += imageSize;
  text += " bytes";

  server.send(200, "text/plain", text);
}

// ---------- Main ----------
void setup() {
  Serial.begin(115200);
  delay(1000);

  cameraInit();
  Serial.println("Camera init OK");

  wifiConnect();

  server.on("/", handleRoot);
  server.on("/capture", handleCapture);
  server.on("/info", handleInfo);
  server.begin();
}

void loop() {
  server.handleClient();
}