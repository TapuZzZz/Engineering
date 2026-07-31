// =================================================================
//  ESP32-CAM  -  FINAL VERSION (persistent retry, dimmed LED)
//  Acts as a TCP SERVER, advertised via mDNS as "esp32cam.local".
//  Keeps retrying the network list FOREVER instead of halting -
//  necessary because of weak signal through concrete walls.
//  Status LED uses PWM dimming (not full digitalWrite HIGH) to
//  avoid heat buildup - this LED is the camera's bright flash LED.
//  Uses the newer ledcAttach()/ledcWrite(pin, ...) API (Arduino
//  ESP32 core 3.x) instead of the older ledcSetup/ledcAttachPin.
// =================================================================

#include <WiFi.h>
#include <ESPmDNS.h>
#include "esp_camera.h"

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
//  Status LED - PWM dimmed instead of full digitalWrite HIGH.
//  This is the camera's bright flash LED, meant for brief full-power
//  bursts, not sustained use - dimming keeps it cool even when left
//  on for a few seconds.
// -----------------------------------------------------------------
#define STATUS_LED_PIN 4
const int LED_PWM_FREQUENCY  = 5000;   // 5kHz - fast enough to avoid visible flicker
const int LED_PWM_RESOLUTION = 8;      // 8-bit = values from 0 (off) to 255 (full brightness)
const int LED_DIM_BRIGHTNESS = 12;     // ~5% brightness - visible but stays cool to the touch

// -----------------------------------------------------------------
//  Known Wi-Fi networks - tries each one in order, cycling forever
//  until one connects. Add more rows here as needed.
// -----------------------------------------------------------------
struct WifiNetwork {
  const char* ssid;
  const char* password;
};

WifiNetwork knownNetworks[] = {
  {"Lagami",                   "0547692269"},
  {"TP-Link- Salon",           "03032007"},
  {"Artyom Kiselhof's iPhone", "artyom0303"}
};

const uint16_t SERVER_PORT = 8000;

WiFiServer tcpServer(SERVER_PORT);
WiFiClient pcConnection;


// -----------------------------------------------------------------
//  Function: connectToWiFi
//  What it does: keeps cycling through knownNetworks[] FOREVER
//  until one connects. Dims the status LED while trying (blinking
//  at low brightness), gives a brief dim confirmation flash once
//  connected, then turns it off.
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
      while (WiFi.status() != WL_CONNECTED && attempts < 30) {   // ~15 seconds for weak signal
        delay(500);
        Serial.print(".");
        ledcWrite(STATUS_LED_PIN, (attempts % 2) ? LED_DIM_BRIGHTNESS : 0);
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

        ledcWrite(STATUS_LED_PIN, LED_DIM_BRIGHTNESS);   // Brief dim confirmation
        delay(500);
        ledcWrite(STATUS_LED_PIN, 0);                     // Then off
        return;
      }

      Serial.println("");
      Serial.print("Failed to connect to: ");
      Serial.println(knownNetworks[i].ssid);
    }

    Serial.println("Completed a full round with no success - trying again...");
    ledcWrite(STATUS_LED_PIN, 0);
    delay(1000);
  }
}


// -----------------------------------------------------------------
//  Function: initCamera
//  What it does: configures the camera driver. Uses TWO frame
//  buffers so the sensor can write a new frame into one buffer
//  while the other is still being read/sent.
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
  config.fb_count      = 2;                     // Two buffers prevent write/read overlap
  config.grab_mode     = CAMERA_GRAB_LATEST;    // Always hand us the newest complete frame

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
//  quality setting runs ~10,000-18,000 bytes, so anything much
//  bigger is almost certainly two frames stuck together.
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

  // Set up PWM on the status LED pin - newer core 3.x API:
  // ledcAttach(pin, freq, resolution) combines what used to be two
  // separate calls (ledcSetup + ledcAttachPin), and ledcWrite takes
  // the pin number directly instead of a channel number.
  ledcAttach(STATUS_LED_PIN, LED_PWM_FREQUENCY, LED_PWM_RESOLUTION);
  ledcWrite(STATUS_LED_PIN, 0);   // Start fully off

  connectToWiFi();

  delay(200);   // Brief settle time before camera init, reduces I2C warnings on wake-up

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