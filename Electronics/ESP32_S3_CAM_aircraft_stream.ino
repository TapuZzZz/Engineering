// =================================================================
//  ESP32-S3-CAM  -  AIRCRAFT DETECTION STREAMING FIRMWARE
//  Rebuilt from scratch for MISA's detection use case (distant,
//  small, sky-background targets) rather than close-range face
//  tracking. Confirmed sensor on this board: OV3660.
//
//  KEY DIFFERENCES FROM THE PREVIOUS (FACE-TRACKING) FIRMWARE:
//    - Higher default resolution (SVGA) - distant aircraft are a
//      small number of pixels; more detail matters more than raw
//      frame rate here.
//    - Sensor tuned for bright-sky backgrounds: reduced exposure
//      bias so the sky doesn't wash out and hide the aircraft
//      silhouette, plus increased sharpness for edge/contour
//      definition.
//    - Bad-pixel/white-pixel correction explicitly DISABLED - a
//      distant aircraft can look like a small number of outlier
//      pixels, and these "cleanup" algorithms are designed to
//      remove exactly that kind of thing.
//    - Self-healing: if several consecutive frame captures fail,
//      the camera driver is torn down and reinitialized instead of
//      leaving the board in a dead state or silently dropping the
//      PC connection.
//    - Periodic free-heap / free-PSRAM reporting to Serial, so a
//      memory-pressure crash (as opposed to a power/brownout
//      crash) is visible in the log instead of just "connection
//      lost" on the PC side.
//
//  PROTOCOL: unchanged - 4-byte little-endian length prefix
//  followed by raw JPEG bytes per frame, over a plain TCP socket
//  on SERVER_PORT. This keeps MISA_Main_Controller.py compatible
//  without changes.
//
//  NOTE ON THE "CONNECTION LOST IMMEDIATELY" SYMPTOM: if frames
//  never start (Serial never prints "Sent frame..."), the most
//  likely cause is NOT software - it's the power supply to this
//  board (brownout under camera+WiFi load). This firmware cannot
//  fix a hardware power problem; it can only make the failure
//  mode visible and recoverable instead of silent.
// =================================================================

#include <WiFi.h>
#include <ESPmDNS.h>
#include "esp_camera.h"
#include "esp_heap_caps.h"

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

// -----------------------------------------------------------------
//  Aircraft-detection tuning knobs - change these to trade off
//  detail vs. frame rate. FRAMESIZE_SVGA (800x600) is a balanced
//  default; go up to FRAMESIZE_XGA (1024x768) or FRAMESIZE_UXGA
//  (1600x1200) for more distant/small targets at the cost of fps
//  and higher bandwidth over WiFi.
// -----------------------------------------------------------------
const framesize_t CAMERA_FRAME_SIZE = FRAMESIZE_VGA;
const int         CAMERA_JPEG_QUALITY = 4;   // Found via live tuning - user confirmed smooth playback at this quality

bool cameraIsReady = false;
unsigned long lastCameraRetryTime = 0;
const unsigned long CAMERA_RETRY_INTERVAL_MS = 4000;

int consecutiveCaptureFailures = 0;
const int MAX_CONSECUTIVE_FAILURES_BEFORE_REINIT = 5;

unsigned long lastMemoryReportTime = 0;
const unsigned long MEMORY_REPORT_INTERVAL_MS = 5000;


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


void describeCameraError(esp_err_t result) {
  Serial.print("Camera init failed. Error code: 0x");
  Serial.println(result, HEX);

  switch (result) {
    case ESP_ERR_NOT_FOUND:
      Serial.println("-> ESP_ERR_NOT_FOUND: sensor not detected on the SCCB/I2C bus.");
      break;
    case ESP_ERR_NO_MEM:
      Serial.println("-> ESP_ERR_NO_MEM: PSRAM/memory allocation failed.");
      Serial.println("   Check Tools > PSRAM > OPI PSRAM is enabled.");
      break;
    case ESP_ERR_INVALID_ARG:
      Serial.println("-> ESP_ERR_INVALID_ARG: invalid pin configuration.");
      break;
    default:
      Serial.println("-> Unrecognized code - look it up against esp_err_t / esp_camera.h.");
      break;
  }
}


// -----------------------------------------------------------------
//  Function: tuneSensorForAircraftDetection
//  What it does: applies sensor-level settings suited to a bright
//  sky background and small, low-contrast distant targets, rather
//  than the close-range portrait tuning used for face tracking.
// -----------------------------------------------------------------
void tuneSensorForAircraftDetection(sensor_t* s) {
  // Settings below were found empirically via the live tuning tool
  // (ESP32_S3_CAM_TuningServer.ino) and confirmed smooth by the user
  // at VGA/quality=4. Indoor-lighting values - see note on AE_LEVEL
  // below for the outdoor/real-deployment adjustment needed.

  // Manual exposure - found to give better brightness than pure
  // gain increases, with less noise/flicker than pushing gain alone.
  s->set_exposure_ctrl(s, 0);
  s->set_aec_value(s, 550);   // Lowered from 699 - slightly less motion blur, still bright enough per user

  s->set_gain_ctrl(s, 1);                    // Auto gain, to fine-tune around the manual exposure.
  s->set_gainceiling(s, GAINCEILING_64X);

  // AE_LEVEL=+1 was needed for dim INDOOR testing. In actual outdoor
  // deployment (bright sky background), this should likely go back
  // toward -1 (the original aircraft-detection default) to avoid
  // saturating the sky - re-test outdoors before relying on this.
  s->set_ae_level(s, 1);

  s->set_sharpness(s, 1);
  s->set_contrast(s, 1);
  s->set_saturation(s, 0);

  // Kept OFF per the aircraft-detection design: BPC/WPC "cleanup"
  // algorithms can erase a small distant aircraft that looks like a
  // few outlier pixels. Confirmed with the user to keep this
  // decision even though it made no visible difference in the
  // close-range indoor test.
  s->set_bpc(s, 0);
  s->set_wpc(s, 0);
  s->set_denoise(s, 0);

  Serial.println("Sensor tuned for aircraft/sky detection (live-tuned settings baked in).");
}


bool tryInitCamera() {
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

  // XCLK controls the sensor's internal pixel readout speed - it was
  // THE dominant factor in the original ~190ms capture time (5fps).
  // History: 10MHz was the original safe value (avoided color/noise
  // artifacts on this OV3660 unit). 16MHz got FPS to target (~21fps)
  // but caused "Send failed" / dropped connections - likely a
  // power/brownout issue under the higher current draw (a documented
  // issue on this project before). 14MHz is the current compromise:
  // still a large speed improvement over 10MHz, with more safety
  // margin than 16MHz. If instability returns even at 14MHz, the
  // next lever is NOT XCLK - it's checking/upgrading the physical
  // power supply, since we may be at this board's current ceiling
  // regardless of clock speed.
  // FINAL DECISION: 16MHz is NOT usable on this board - confirmed
  // unstable in TWO independent tests: once with the DevKit sharing
  // power, and once with the camera completely isolated on its own
  // supply (this second test ruled out shared-power as the cause).
  // Both times: near-immediate "Send failed" / multi-second send
  // stalls. This is an intrinsic limit of this board/sensor unit at
  // 16MHz, not a wiring or software issue we can fix by tuning
  // exposure or JPEG quality (both were tested and had no effect on
  // the underlying stall). 14MHz is the reliable ceiling for this
  // hardware. Further FPS gains beyond what 14MHz gives will need a
  // hardware-side investigation (measuring this board's own power
  // rail under load, trying a different USB cable/port or a
  // dedicated 5V/2A supply) rather than more firmware tuning.
  config.xclk_freq_hz  = 14000000;
  config.pixel_format  = PIXFORMAT_JPEG;
  config.frame_size    = CAMERA_FRAME_SIZE;
  config.jpeg_quality  = CAMERA_JPEG_QUALITY;
  config.fb_count      = 2;
  config.grab_mode     = CAMERA_GRAB_LATEST;
  config.fb_location   = CAMERA_FB_IN_PSRAM;

  esp_err_t result = esp_camera_init(&config);

  if (result != ESP_OK) {
    describeCameraError(result);
    return false;
  }

  Serial.println("Camera initialized successfully.");

  sensor_t * s = esp_camera_sensor_get();
  Serial.print("Detected sensor PID: 0x");
  Serial.println(s->id.PID, HEX);

  if (s->id.PID == OV3660_PID) {
    Serial.println("OV3660 sensor confirmed.");
    s->set_vflip(s, 1);   // Physical mounting correction, confirmed on this board.
  }

  tuneSensorForAircraftDetection(s);

  return true;
}


// -----------------------------------------------------------------
//  Function: reinitCameraAfterFailure
//  What it does: tears down and rebuilds the camera driver after
//  repeated consecutive frame-capture failures, instead of leaving
//  the board stuck or silently dropping the client. This is a
//  software-level recovery attempt only - it will NOT fix a real
//  power/brownout problem, but it prevents a transient glitch from
//  requiring a manual power cycle.
// -----------------------------------------------------------------
void reinitCameraAfterFailure() {
  Serial.println("Too many consecutive frame failures - reinitializing camera driver...");
  esp_camera_deinit();
  delay(200);
  cameraIsReady = tryInitCamera();
  consecutiveCaptureFailures = 0;
  lastCameraRetryTime = millis();
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
  // Raised ceiling vs. the VGA face-tracking version, since SVGA/XGA
  // JPEGs are legitimately larger.
  const size_t MAX_REASONABLE_SIZE = 90000;

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
    consecutiveCaptureFailures++;
    return;
  }

  if (!isValidJpeg(frameBuffer->buf, frameBuffer->len)) {
    Serial.print("Discarded corrupt frame, size was: ");
    Serial.println(frameBuffer->len);
    esp_camera_fb_return(frameBuffer);
    consecutiveCaptureFailures++;
    return;
  }

  consecutiveCaptureFailures = 0;

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


void reportMemoryStatus() {
  Serial.print("[MEM] Free heap: ");
  Serial.print(heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  Serial.print(" bytes | Free PSRAM: ");
  Serial.print(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  Serial.println(" bytes");
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

  // TCP server starts regardless of camera status, so the board is
  // always reachable from the PC even if the camera needs a retry.
  tcpServer.begin();
  Serial.println("TCP server started.");

  cameraIsReady = tryInitCamera();
  lastCameraRetryTime = millis();

  if (!cameraIsReady) {
    Serial.println("Camera not ready yet - will keep retrying every 4 seconds.");
  }
}

void loop() {
  checkWiFiStillConnected();

  if (millis() - lastMemoryReportTime >= MEMORY_REPORT_INTERVAL_MS) {
    reportMemoryStatus();
    lastMemoryReportTime = millis();
  }

  if (!cameraIsReady) {
    if (millis() - lastCameraRetryTime >= CAMERA_RETRY_INTERVAL_MS) {
      Serial.println("Retrying camera init...");
      cameraIsReady = tryInitCamera();
      lastCameraRetryTime = millis();
    }
    delay(50);
    return;
  }

  if (consecutiveCaptureFailures >= MAX_CONSECUTIVE_FAILURES_BEFORE_REINIT) {
    reinitCameraAfterFailure();
    return;
  }

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
  delay(1);
}
