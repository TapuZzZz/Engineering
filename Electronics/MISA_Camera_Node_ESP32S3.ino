// =================================================================
//  ESP32-S3-CAM  -  FINAL VERSION (new 5MP camera board)
//  Acts as a TCP SERVER, advertised via mDNS as "esp32cam.local".
//  Tries each known Wi-Fi network in order until one connects.
//
//  *** IMPORTANT - PIN MAPPING STATUS ***
//  This board was purchased without confirmed official pin
//  documentation. The camera pin numbers below are the
//  "CAMERA_MODEL_ESP32S3_EYE" mapping - the configuration most
//  commonly cited across multiple independent sources (Arduino
//  Forum, Espressif GitHub discussions) for this exact style of
//  generic ESP32-S3-WROOM CAM board (FPC ribbon camera connector,
//  dual USB-C, EN/RST + BOOT buttons, N16R8 module).
//  This is the best available estimate, NOT a confirmed fact.
//  BEFORE relying on this in the final project: verify each pin
//  with a multimeter in continuity mode between each FPC connector
//  contact and the corresponding labeled GPIO pin on the board
//  edge. If frame capture fails or the image looks wrong, this
//  pin map is the first thing to re-check.
// =================================================================

#include <WiFi.h>
#include <ESPmDNS.h>
#include "esp_camera.h"

// -----------------------------------------------------------------
//  Camera pin map - ESP32S3_EYE-style (best available estimate,
//  see warning above). PWDN and RESET are not used on this board
//  design (-1 means "not connected / not needed").
// -----------------------------------------------------------------
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

// -----------------------------------------------------------------
//  Known Wi-Fi networks - tries each one in order, cycling forever
//  until one connects. Add more rows here as needed.
// -----------------------------------------------------------------
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


// -----------------------------------------------------------------
//  Function: connectToWiFi
//  What it does: keeps cycling through knownNetworks[] FOREVER
//  until one connects, resetting the Wi-Fi driver state fully
//  before each attempt to avoid the "cannot set config" error
//  seen when switching networks too quickly.
// -----------------------------------------------------------------
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


// -----------------------------------------------------------------
//  Function: initCamera
//  What it does: configures the camera driver. Resolution is
//  DELIBERATELY kept at VGA (640x480), not the sensor's full 5MP
//  capability - this is a conscious decision carried over from
//  earlier testing: higher resolution means larger JPEG frames,
//  which means more data to transmit over Wi-Fi, which directly
//  hurt tracking latency on weaker signal connections. The 5MP
//  sensor gives us that option for the future, not an obligation
//  to use it now. Two frame buffers + CAMERA_GRAB_LATEST prevent
//  the buffer-tearing (merged/corrupt frames) issue solved earlier.
//  fb_location is explicitly set to PSRAM, since this board has
//  8MB of it available - safer than relying on limited internal SRAM.
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

  config.xclk_freq_hz  = 20000000;
  config.pixel_format  = PIXFORMAT_JPEG;
  config.frame_size    = FRAMESIZE_VGA;         // 640x480 - deliberate choice, see comment above
  config.jpeg_quality  = 10;
  config.fb_count      = 2;
  config.grab_mode     = CAMERA_GRAB_LATEST;
  config.fb_location   = CAMERA_FB_IN_PSRAM;    // NEW: this board has 8MB PSRAM, use it

  esp_err_t result = esp_camera_init(&config);

  if (result != ESP_OK) {
    Serial.print("Camera init failed with error code: ");
    Serial.println(result);
    Serial.println("If this fails, the pin mapping above is the most likely cause - verify with a multimeter.");
    return false;
  }

  Serial.println("Camera initialized successfully.");
  return true;
}


// -----------------------------------------------------------------
//  Function: sendAllBytes
//  What it does: TCP write() is not guaranteed to send every byte
//  in one call. This function keeps sending the remaining bytes
//  until everything is out, and correctly detects failure using a
//  SIGNED type (so a -1 error return doesn't silently wrap around
//  into a huge positive number).
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
//  What it does: every valid JPEG file must start with bytes
//  0xFF 0xD8 and end with bytes 0xFF 0xD9. We also reject frames
//  that are unusually large - a normal single VGA JPEG at our
//  quality setting runs roughly 10,000-18,000 bytes, so anything
//  much bigger is almost certainly two frames stuck together in
//  the camera's shared buffer.
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
//  What it does: grabs one JPEG frame, discards it if it's corrupt
//  or oversized, otherwise sends it to the PC and measures timing.
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


// -----------------------------------------------------------------
//  Function: checkWiFiStillConnected
//  What it does: with a weak/unstable signal, the connection can
//  drop mid-session, not just at startup. This checks every loop
//  iteration and re-runs the full connect procedure if we've lost
//  the network.
// -----------------------------------------------------------------
void checkWiFiStillConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi connection lost - reconnecting...");
    connectToWiFi();
    tcpServer.begin();
  }
}


// =================================================================
//  MAIN PROGRAM
// =================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  connectToWiFi();

  delay(200);   // Brief settle time before camera init

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