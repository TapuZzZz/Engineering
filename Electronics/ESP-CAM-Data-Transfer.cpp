// =================================================================
//  ESP32-CAM  -  FINAL VERSION (multi-network, robust Wi-Fi retry,
//  LED status indicator)
//  Acts as a TCP SERVER, advertised via mDNS as "esp32cam.local".
//  Tries each known Wi-Fi network in order until one connects.
//  The built-in flash LED turns on solid once connected, so status
//  is visible even without a Serial Monitor attached.
// =================================================================

#include <WiFi.h>
#include <ESPmDNS.h>
#include "esp_camera.h"   // Library that controls the OV2640 camera chip itself

// -----------------------------------------------------------------
//  Camera pin map - fixed by the physical wiring of the
//  "AI Thinker ESP32-CAM" board.
// -----------------------------------------------------------------
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// -----------------------------------------------------------------
//  Status LED - the built-in white flash LED on AI Thinker
//  ESP32-CAM. Turns on solid once Wi-Fi connects successfully, so
//  connection status is visible without a Serial Monitor attached.
// -----------------------------------------------------------------
#define STATUS_LED_PIN 4

// -----------------------------------------------------------------
//  Known Wi-Fi networks - the board tries each one in order until
//  it successfully connects. Add more rows here as needed.
// -----------------------------------------------------------------
struct WifiNetwork {
  const char* ssid;
  const char* password;
};

WifiNetwork knownNetworks[] = {
  {"TP-Link- Salon",           "03032007"},
  {"Lagami",                   "0547692269"},
  {"Artyom Kiselhof's iPhone", "artyom0303"}
};

const uint16_t SERVER_PORT = 8000;

WiFiServer tcpServer(SERVER_PORT);
WiFiClient pcConnection;


// -----------------------------------------------------------------
//  Function: connectToWiFi
//  What it does: explicitly resets the Wi-Fi driver's internal
//  state before each connection attempt, which fixes the
//  "sta is connecting, cannot set config" error seen when switching
//  networks too quickly. Tries each network in knownNetworks[] in
//  order. Turns the status LED on solid once connected.
// -----------------------------------------------------------------
void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  delay(100);

  int networkCount = sizeof(knownNetworks) / sizeof(knownNetworks[0]);

  for (int i = 0; i < networkCount; i++) {
    Serial.print("Trying network: ");
    Serial.println(knownNetworks[i].ssid);

    WiFi.disconnect(true, true);
    delay(300);

    WiFi.begin(knownNetworks[i].ssid, knownNetworks[i].password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("Connected!");
      Serial.print("Network: ");
      Serial.println(knownNetworks[i].ssid);
      Serial.print("ESP32-CAM IP address: ");
      Serial.println(WiFi.localIP());

      if (MDNS.begin("esp32cam")) {
        Serial.println("mDNS responder started: esp32cam.local");
      } else {
        Serial.println("mDNS setup failed - PC will need the raw IP instead.");
      }

      digitalWrite(STATUS_LED_PIN, HIGH);   // NEW: solid ON = connected successfully
      return;
    }

    Serial.println("");
    Serial.print("Failed to connect to: ");
    Serial.println(knownNetworks[i].ssid);
  }

  Serial.println("Could not connect to any known network. Halting.");
  // LED stays OFF here - never turned on because we never connected
  while (true) { delay(1000); }
}


// -----------------------------------------------------------------
//  Function: initCamera
// -----------------------------------------------------------------
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

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_VGA;
  config.jpeg_quality  = 10;
  config.fb_count      = 2;
  config.grab_mode     = CAMERA_GRAB_LATEST;

  esp_err_t result = esp_camera_init(&config);

  if (result != ESP_OK) {
    Serial.print("Camera init failed with error code: ");
    Serial.println(result);
    return false;
  }

  Serial.println("Camera initialized successfully.");
  return true;
}


// -----------------------------------------------------------------
//  Function: sendAllBytes
// -----------------------------------------------------------------
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


// -----------------------------------------------------------------
//  Function: isValidJpeg
// -----------------------------------------------------------------
bool isValidJpeg(const uint8_t* data, size_t length) {
  const size_t MAX_REASONABLE_SIZE = 25000;

  if (length < 4 || length > MAX_REASONABLE_SIZE) {
    return false;
  }

  bool startsCorrectly = (data[0] == 0xFF && data[1] == 0xD8);
  bool endsCorrectly   = (data[length - 2] == 0xFF && data[length - 1] == 0xD9);

  return startsCorrectly && endsCorrectly;
}


// -----------------------------------------------------------------
//  Function: captureAndSendFrame
// -----------------------------------------------------------------
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


// =================================================================
//  MAIN PROGRAM
// =================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);   // Off until connected

  connectToWiFi();

  if (!initCamera()) {
    Serial.println("Stopping - camera did not initialize.");
    while (true) { delay(1000); }
  }

  tcpServer.begin();
  Serial.println("Camera server ready, waiting for PC to connect...");
}

void loop() {
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