// =================================================================
//  ESP32-S3-CAM  -  FINAL VERSION (OV3660 sensor)
//  Acts as a TCP SERVER, advertised via mDNS as "esp32cam.local".
//  Tries each known Wi-Fi network in order until one connects.
//
//  Confirmed working configuration:
//    - PSRAM frame buffers (requires Tools > PSRAM > OPI PSRAM,
//      and Tools > Flash Size > 16MB in the Arduino IDE menus)
//    - XCLK at 10MHz - confirmed to eliminate color/noise artifacts
//      seen at the default 20MHz on this board+sensor combination
//    - OV3660-specific color correction (documented Espressif fix)
//    - Frame size sanity threshold raised for this sensor's larger
//      VGA JPEG output
//
//  NOTE: this firmware calls set_vflip(1) for OV3660, so the image
//  arrives at the PC flipped 180 degrees. The Python-side
//  decode_frame() function must apply cv2.rotate(image,
//  cv2.ROTATE_180) to compensate - already done in all project
//  Python files.
// =================================================================

#include <WiFi.h>
#include <ESPmDNS.h>
#include "esp_camera.h"

#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM      4
#define SIOC_GPIO_NUM      5
#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM        8
#define Y3_GPIO_NUM        9
#define Y2_GPIO_NUM       11
#define VSYNC_GPIO_NUM     6
#define HREF_GPIO_NUM      7
#define PCLK_GPIO_NUM     13

struct WifiNetwork {
  const char* ssid;
  const char* password;
};

WifiNetwork knownNetworks[] = {
  {"TP-Link- Salon",           "03032007"},
  {"Artyom Kiselhof's iPhone", "artyom0303"},
  {"Lagami",                   "0547692269"}
};

const uint16_t SERVER_PORT = 8000;

WiFiServer tcpServer(SERVER_PORT);
WiFiClient pcConnection;


void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  delay(100);

  int networkCount = sizeof(knownNetworks) / sizeof(knownNetworks[0]);
  int roundNumber = 0;

  while (true) {
    roundNumber++;
    Serial.print("--- Connection round ");
    Serial.print(roundNumber);
    Serial.println(" ---");

    for (int i = 0; i < networkCount; i++) {
      Serial.print("Trying network: ");
      Serial.println(knownNetworks[i].ssid);

      WiFi.disconnect(true, true);
      delay(300);

      WiFi.begin(knownNetworks[i].ssid, knownNetworks[i].password);

      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("Connected!");
        Serial.print("Network: ");
        Serial.println(knownNetworks[i].ssid);
        Serial.print("ESP32-S3-CAM IP address: ");
        Serial.println(WiFi.localIP());

        if (MDNS.begin("esp32cam")) {
          Serial.println("mDNS responder started: esp32cam.local");
        } else {
          Serial.println("mDNS setup failed - PC will need the raw IP instead.");
        }

        return;
      }

      Serial.println("");
      Serial.print("Failed to connect to: ");
      Serial.println(knownNetworks[i].ssid);
    }

    Serial.println("Completed a full round with no success - trying again...");
    delay(1000);
  }
}


bool initCamera() {
  camera_config_t config;

  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.xclk_freq_hz  = 10000000;
  config.pixel_format  = PIXFORMAT_JPEG;
  config.frame_size    = FRAMESIZE_VGA;
  config.jpeg_quality  = 12;
  config.fb_count      = 2;
  config.grab_mode     = CAMERA_GRAB_LATEST;
  config.fb_location   = CAMERA_FB_IN_PSRAM;

  esp_err_t result = esp_camera_init(&config);

  if (result != ESP_OK) {
    Serial.print("Camera init failed with error code: ");
    Serial.println(result);
    Serial.println("Check: Tools > PSRAM > OPI PSRAM must be enabled in Arduino IDE.");
    return false;
  }

  Serial.println("Camera initialized successfully.");

  sensor_t * s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    Serial.println("OV3660 sensor detected - applying color correction.");
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  } else {
    Serial.println("Non-OV3660 sensor detected - skipping OV3660-specific correction.");
  }

  return true;
}


void sendAllBytes(const uint8_t* data, size_t totalLength) {
  size_t sentSoFar = 0;

  while (sentSoFar < totalLength) {
    int justSent = pcConnection.write(data + sentSoFar, totalLength - sentSoFar);

    if (justSent <= 0) {
      Serial.println("Send failed - connection may be broken.");
      pcConnection.stop();
      return;
    }

    sentSoFar += (size_t)justSent;
  }
}


bool isValidJpeg(const uint8_t* data, size_t length) {
  const size_t MAX_REASONABLE_SIZE = 35000;

  if (length < 4 || length > MAX_REASONABLE_SIZE) {
    return false;
  }

  bool startsCorrectly = (data[0] == 0xFF && data[1] == 0xD8);
  bool endsCorrectly   = (data[length - 2] == 0xFF && data[length - 1] == 0xD9);

  return startsCorrectly && endsCorrectly;
}


void captureAndSendFrame() {
  unsigned long startTime = millis();

  camera_fb_t* frameBuffer = esp_camera_fb_get();

  if (!frameBuffer) {
    Serial.println("Frame capture failed.");
    return;
  }

  if (!isValidJpeg(frameBuffer->buf, frameBuffer->len)) {
    Serial.print("Discarded corrupt frame, size was: ");
    Serial.println(frameBuffer->len);
    esp_camera_fb_return(frameBuffer);
    return;
  }

  unsigned long captureTime = millis();

  uint32_t frameSize = frameBuffer->len;

  sendAllBytes((uint8_t*)&frameSize, sizeof(frameSize));
  sendAllBytes(frameBuffer->buf, frameBuffer->len);

  unsigned long sendTime = millis();

  esp_camera_fb_return(frameBuffer);

  Serial.print("Sent frame, size: ");
  Serial.print(frameSize);
  Serial.print(" bytes | capture: ");
  Serial.print(captureTime - startTime);
  Serial.print(" ms | send: ");
  Serial.print(sendTime - captureTime);
  Serial.println(" ms");
}


void checkWiFiStillConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi connection lost - reconnecting...");
    connectToWiFi();
    tcpServer.begin();
  }
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  connectToWiFi();

  delay(200);

  if (!initCamera()) {
    Serial.println("Stopping - camera did not initialize.");
    while (true) { delay(1000); }
  }

  tcpServer.begin();
  Serial.println("Camera server ready, waiting for PC to connect...");
}

void loop() {
  checkWiFiStillConnected();

  if (!pcConnection || !pcConnection.connected()) {
    pcConnection = tcpServer.available();
    if (!pcConnection) {
      delay(50);
      return;
    }
    pcConnection.setNoDelay(true);
    Serial.println("PC connected.");
  }

  captureAndSendFrame();
  delay(25);
}