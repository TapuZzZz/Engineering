// =================================================================
//  ESP32-CAM  -  PART 3: Send Frames to PC over TCP
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

const char* PC_IP_ADDRESS = "192.168.0.173";  // <-- Replace XXX with your Mac's real IP
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
  config.frame_size   = FRAMESIZE_QVGA;
  config.jpeg_quality  = 12;
  config.fb_count      = 1;

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
//  connected. Returns true only if a connection is ready to use.
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

  Serial.println("Connected to PC server.");
  return true;
}


// -----------------------------------------------------------------
//  Function: captureAndSendFrame
//  What it does: grabs one JPEG frame and sends it to the PC as
//  [4 bytes = size][JPEG bytes], so the PC knows exactly how many
//  bytes belong to this frame.
// -----------------------------------------------------------------
void captureAndSendFrame() {
  camera_fb_t* frameBuffer = esp_camera_fb_get();

  if (!frameBuffer) {
    Serial.println("Frame capture failed.");
    return;
  }

  uint32_t frameSize = frameBuffer->len;

  tcpClient.write((uint8_t*)&frameSize, sizeof(frameSize));
  tcpClient.write(frameBuffer->buf, frameBuffer->len);

  esp_camera_fb_return(frameBuffer);

  Serial.print("Sent frame, size in bytes: ");
  Serial.println(frameSize);
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
  delay(100);   // ~10 frames per second
}