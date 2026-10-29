#include "esp_camera.h"

// ==========================================
// Settings
// ==========================================
#define SERIAL_BAUD        115200
#define CAM_XCLK_FREQ_HZ   20000000
#define CAM_JPEG_QUALITY   10
#define CAM_FB_COUNT       2
#define TARGET_FPS         20
#define FRAME_INTERVAL_MS  (1000 / TARGET_FPS)

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

// ==========================================
// Camera Config
// ==========================================
void setControlPins(camera_config_t& config) {
  config.pin_pwdn     = CAM_PIN_PWDN;
  config.pin_reset    = CAM_PIN_RESET;
  config.pin_xclk     = CAM_PIN_XCLK;
  config.pin_sccb_sda = CAM_PIN_SIOD;
  config.pin_sccb_scl = CAM_PIN_SIOC;
  config.pin_vsync    = CAM_PIN_VSYNC;
  config.pin_href     = CAM_PIN_HREF;
  config.pin_pclk     = CAM_PIN_PCLK;
}

void setDataPins(camera_config_t& config) {
  config.pin_d0 = CAM_PIN_D0;
  config.pin_d1 = CAM_PIN_D1;
  config.pin_d2 = CAM_PIN_D2;
  config.pin_d3 = CAM_PIN_D3;
  config.pin_d4 = CAM_PIN_D4;
  config.pin_d5 = CAM_PIN_D5;
  config.pin_d6 = CAM_PIN_D6;
  config.pin_d7 = CAM_PIN_D7;
}

void setImageSettings(camera_config_t& config) {
  config.xclk_freq_hz = CAM_XCLK_FREQ_HZ;
  config.ledc_timer   = LEDC_TIMER_0;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_VGA;
  config.jpeg_quality = CAM_JPEG_QUALITY;
  config.fb_count     = CAM_FB_COUNT;
  config.fb_location  = CAMERA_FB_IN_PSRAM;
  config.grab_mode    = CAMERA_GRAB_LATEST;
}

bool initCamera() {
  camera_config_t config = {};
  setControlPins(config);
  setDataPins(config);
  setImageSettings(config);
  return esp_camera_init(&config) == ESP_OK;
}

// ==========================================
// Sensor Quality
// ==========================================
void applySensorSettings() {
  sensor_t* sensor = esp_camera_sensor_get();
  if (sensor == nullptr) {
    Serial.println("Sensor not found");
    return;
  }
  Serial.printf("Sensor PID: 0x%x\n", sensor->id.PID);

  sensor->set_saturation(sensor, 0);
  sensor->set_contrast(sensor, 0);
  sensor->set_whitebal(sensor, 1);
  sensor->set_awb_gain(sensor, 1);
  sensor->set_wb_mode(sensor, 0);
  sensor->set_gainceiling(sensor, GAINCEILING_4X);

  sensor->set_exposure_ctrl(sensor, 1);
  sensor->set_aec2(sensor, 1);
  sensor->set_gain_ctrl(sensor, 1);

  sensor->set_bpc(sensor, 1);
  sensor->set_wpc(sensor, 1);
  sensor->set_raw_gma(sensor, 1);
  sensor->set_lenc(sensor, 1);
  sensor->set_dcw(sensor, 1);

  sensor->set_vflip(sensor, 1);
  sensor->set_hmirror(sensor, 1);
}

// ==========================================
// Capture
// ==========================================
bool isTimeForNextFrame() {
  static unsigned long lastFrameMs = 0;
  unsigned long nowMs = millis();
  if (nowMs - lastFrameMs < FRAME_INTERVAL_MS) return false;
  lastFrameMs = nowMs;
  return true;
}

void printFrameInfo(const camera_fb_t* frame) {
  Serial.printf("[%lu ms] %ux%u, %u bytes, %.1f C\n",
                millis(), frame->width, frame->height, frame->len,
                temperatureRead());
}

void captureFrame() {
  camera_fb_t* frame = esp_camera_fb_get();
  if (frame == nullptr) {
    Serial.println("Capture failed");
    return;
  }
  printFrameInfo(frame);
  esp_camera_fb_return(frame);
}

// ==========================================
// Main
// ==========================================
void setup() {
  Serial.begin(SERIAL_BAUD);

  if (!initCamera()) {
    Serial.println("Camera init failed");
    return;
  }

  applySensorSettings();
  Serial.println("CAM ready");
}

void loop() {
  if (!isTimeForNextFrame()) return;
  captureFrame();
}