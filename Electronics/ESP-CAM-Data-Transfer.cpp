// =================================================================
//  ESP32-CAM  -  PART 3g: Send Frames to PC over TCP (dual buffer + size check)
// =================================================================

#include <WiFi.h>
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
//  Network configuration
// -----------------------------------------------------------------
const char* WIFI_SSID     = "TP-Link- Salon";
const char* WIFI_PASSWORD = "03032007";

const char* PC_IP_ADDRESS = "192.168.0.173";
const uint16_t PC_PORT    = 8000;

WiFiClient tcpClient;   // One reusable TCP connection object


// -----------------------------------------------------------------
//  Function: connectToWiFi
// -----------------------------------------------------------------
void connectToWiFi() {
  Serial.println("Connecting to network...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connected!");
  Serial.print("ESP32-CAM IP address: ");
  Serial.println(WiFi.localIP());
}


// -----------------------------------------------------------------
//  Function: initCamera
//  What it does: configures the camera driver. Uses TWO frame
//  buffers so the sensor can write a new frame into one buffer
//  while the other is still being read/sent - this prevents the
//  buffer-tearing (merged frames) we saw with a single buffer.
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
  config.frame_size   = FRAMESIZE_VGA;         // 640x480
  config.jpeg_quality  = 10;                    // Lower = higher quality, less compression artifacts
  config.fb_count      = 2;                     // CHANGED: was 1 - two buffers prevent write/read overlap
  config.grab_mode     = CAMERA_GRAB_LATEST;    // NEW: always hand us the newest complete frame

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
//  Function: connectToServer
//  What it does: opens a TCP connection to the PC if not already
//  connected, and disables Nagle's algorithm so small writes (like
//  our 4-byte size header) go out immediately.
// -----------------------------------------------------------------
bool connectToServer() {
  if (tcpClient.connected()) {
    return true;
  }

  Serial.println("Connecting to PC server...");
  bool success = tcpClient.connect(PC_IP_ADDRESS, PC_PORT);

  if (!success) {
    Serial.println("Failed to connect to PC server.");
    return false;
  }

  tcpClient.setNoDelay(true);   // Disable Nagle's algorithm - send immediately, don't batch small writes

  Serial.println("Connected to PC server.");
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
    int justSent = tcpClient.write(data + sentSoFar, totalLength - sentSoFar);

    if (justSent <= 0) {   // Catches both 0 AND negative (error) cases
      Serial.println("Send failed - connection may be broken.");
      tcpClient.stop();
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
//  quality setting runs ~10,000-18,000 bytes, so anything much
//  bigger is almost certainly two frames stuck together, even if
//  the start/end markers happen to look valid on their own.
// -----------------------------------------------------------------
bool isValidJpeg(const uint8_t* data, size_t length) {
  const size_t MAX_REASONABLE_SIZE = 25000;   // Sanity ceiling based on frames observed in testing

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
  unsigned long startTime = millis();   // Timestamp before we start capturing

  camera_fb_t* frameBuffer = esp_camera_fb_get();

  if (!frameBuffer) {
    Serial.println("Frame capture failed.");
    return;
  }

  // Reject corrupted/merged frames before wasting time sending them
  if (!isValidJpeg(frameBuffer->buf, frameBuffer->len)) {
    Serial.print("Discarded corrupt frame, size was: ");
    Serial.println(frameBuffer->len);
    esp_camera_fb_return(frameBuffer);   // Still must return it, even though we're skipping it
    return;
  }

  unsigned long captureTime = millis();   // Timestamp right after capture finished

  uint32_t frameSize = frameBuffer->len;

  sendAllBytes((uint8_t*)&frameSize, sizeof(frameSize));   // Send the 4-byte size, fully
  sendAllBytes(frameBuffer->buf, frameBuffer->len);         // Send the JPEG bytes, fully

  unsigned long sendTime = millis();   // Timestamp right after sending finished

  esp_camera_fb_return(frameBuffer);

  // Report how long each stage took, in milliseconds
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

  connectToWiFi();

  if (!initCamera()) {
    Serial.println("Stopping - camera did not initialize.");
    while (true) { delay(1000); }
  }
}

void loop() {
  if (connectToServer()) {
    captureAndSendFrame();
  }
  delay(25);   // Target ~22-23 FPS: ~19ms processing + 25ms delay
}