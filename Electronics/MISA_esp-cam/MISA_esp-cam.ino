#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiUdp.h>

// ===================== Network settings =====================
struct WifiNetwork {
  const char* ssid;
  const char* pass;
};

// Tried in this order
const WifiNetwork NETWORKS[] = {
  { "TP-Link- Salon", "03032007" },
  { "Amal-Student",   "Amal1@st" },
};
const int NETWORK_COUNT = sizeof(NETWORKS) / sizeof(NETWORKS[0]);

const char* HOSTNAME = "esp-cam";
const char* AP_SSID  = "ESP-Cam";   // own network if all WiFi fails
const char* AP_PASS  = "03032007";

#define WIFI_TIMEOUT_MS 10000
#define DISCOVERY_PORT  5005
#define ANNOUNCE_MS     1000
#define BOUNDARY        "frame"

#define MAX_CHIP_TEMP   75.0f   // °C - shut down above this
#define TEMP_CHECK_MS   2000

WiFiServer server(80);
WiFiUDP    udp;

// ===================== Web page =====================
const char PAGE[] = R"(
<!DOCTYPE html>
<html>
<body style="margin:0; background:#000;">
  <img src="/stream" style="display:block; margin:90px auto; transform:rotate(90deg);">
</body>
</html>
)";

// ===================== Camera pins =====================
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

// ===================== Camera config =====================
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
  config.xclk_freq_hz = 20000000;
  config.ledc_timer   = LEDC_TIMER_0;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_HVGA;      // 480x320
  config.jpeg_quality = 10;                  // lower = better quality (10-63)
  config.fb_count     = 2;                   // capture next frame while sending current
  config.fb_location  = CAMERA_FB_IN_PSRAM;
  config.grab_mode    = CAMERA_GRAB_LATEST;  // always send the newest frame
}

camera_config_t buildCameraConfig() {
  camera_config_t config = {};
  setControlPins(config);
  setDataPins(config);
  setImageSettings(config);
  return config;
}

esp_err_t cameraInit() {
  camera_config_t config = buildCameraConfig();
  return esp_camera_init(&config);
}

// ===================== Sensor quality =====================
void applyColorSettings(sensor_t* s) {
  s->set_saturation(s, 2);  // stronger colors
  s->set_contrast(s, 1);    // slightly more contrast
  s->set_whitebal(s, 1);    // auto white balance
  s->set_awb_gain(s, 1);
  s->set_wb_mode(s, 0);     // 0 = auto
}

void applyExposureSettings(sensor_t* s) {
  s->set_exposure_ctrl(s, 1);  // auto exposure
  s->set_aec2(s, 1);           // smarter auto exposure
  s->set_gain_ctrl(s, 1);      // auto gain
}

void applyCorrectionSettings(sensor_t* s) {
  s->set_bpc(s, 1);      // fix black (dead) pixels
  s->set_wpc(s, 1);      // fix white (hot) pixels
  s->set_raw_gma(s, 1);  // gamma correction
  s->set_lenc(s, 1);     // lens correction (darker corners)
  s->set_dcw(s, 1);      // clean downscaling
}

void applySharpness(sensor_t* s) {
  if (s->id.PID != OV5640_PID) return;  // OV2640 has no sharpness control
  s->set_sharpness(s, 1);
}

void applySensorSettings() {
  sensor_t* s = esp_camera_sensor_get();
  if (s == nullptr) return;
  Serial.printf("Sensor PID: 0x%x\n", s->id.PID);
  applyColorSettings(s);
  applyExposureSettings(s);
  applyCorrectionSettings(s);
  applySharpness(s);
}

// ===================== Overheat protection =====================
void shutdownIfOverheated() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < TEMP_CHECK_MS) return;
  lastCheck = millis();
  float temp = temperatureRead();
  if (temp < MAX_CHIP_TEMP) return;
  Serial.printf("Overheat %.1f C - shutting down\n", temp);
  Serial.flush();
  esp_camera_deinit();
  esp_deep_sleep_start();  // no wake source = stays off until RESET
}

// ===================== WiFi =====================
bool tryNetwork(const WifiNetwork& net) {
  Serial.printf("Trying %s", net.ssid);
  WiFi.begin(net.ssid, net.pass);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

bool connectToKnownNetworks() {
  for (int i = 0; i < NETWORK_COUNT; i++) {
    if (tryNetwork(NETWORKS[i])) return true;
    WiFi.disconnect();
  }
  return false;
}

void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("AP mode: join %s, open http://%s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

void wifiConnect() {
  WiFi.setHostname(HOSTNAME);  // must be before mode()
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);        // no power saving = faster, steadier stream
  if (connectToKnownNetworks()) Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
  else startAccessPoint();
}

// ===================== Discovery =====================
void announceIfDue() {
  static unsigned long lastTime = 0;
  if (millis() - lastTime < ANNOUNCE_MS) return;
  lastTime = millis();
  udp.beginPacket(WiFi.broadcastIP(), DISCOVERY_PORT);
  udp.print("HELLO,CAM");
  udp.endPacket();
}

// ===================== HTTP responses =====================
String readRequestLine(WiFiClient& client) {
  String line = client.readStringUntil('\n');
  while (client.available()) client.read();  // drop the rest of the headers
  return line;
}

void sendPage(WiFiClient& client) {
  client.print("HTTP/1.1 200 OK\r\n");
  client.print("Content-Type: text/html\r\n");
  client.print("Connection: close\r\n\r\n");
  client.print(PAGE);
}

void sendStreamHeader(WiFiClient& client) {
  client.print("HTTP/1.1 200 OK\r\n");
  client.print("Content-Type: multipart/x-mixed-replace; boundary=" BOUNDARY "\r\n");
  client.print("Cache-Control: no-cache\r\n\r\n");
}

void sendFrameHeader(WiFiClient& client, size_t len) {
  client.print("--" BOUNDARY "\r\n");
  client.print("Content-Type: image/jpeg\r\n");
  client.printf("Content-Length: %u\r\n\r\n", len);
}

// ===================== Streaming =====================
bool sendFrame(WiFiClient& client) {
  camera_fb_t* fb = esp_camera_fb_get();
  if (fb == nullptr) return true;  // skip this frame, keep streaming
  size_t len = fb->len;
  sendFrameHeader(client, len);
  size_t sent = client.write(fb->buf, len);
  client.print("\r\n");
  esp_camera_fb_return(fb);
  return sent == len;  // false = client disconnected
}

void rejectExtraClients() {
  WiFiClient extra = server.accept();
  if (!extra) return;
  extra.print("HTTP/1.1 503 Service Unavailable\r\n\r\nBusy");
  extra.stop();
}

void printFps() {
  static int frames = 0;
  static unsigned long lastTime = millis();
  frames++;
  if (millis() - lastTime < 1000) return;
  Serial.printf("FPS: %d   chip %.1f C\n", frames, temperatureRead());
  frames = 0;
  lastTime = millis();
}

void streamToClient(WiFiClient& client) {
  sendStreamHeader(client);
  while (client.connected()) {
    if (!sendFrame(client)) break;
    rejectExtraClients();
    printFps();
    shutdownIfOverheated();
  }
}

// ===================== Client handling =====================
void handleClient(WiFiClient& client) {
  client.setNoDelay(true);  // send packets immediately, no waiting
  String request = readRequestLine(client);
  if (request.indexOf("/stream") >= 0) streamToClient(client);
  else sendPage(client);
  client.stop();
}

void haltWithError(const char* message) {
  Serial.println(message);
  while (true) delay(1000);
}

// ===================== Main =====================
void setup() {
  Serial.begin(115200);
  if (cameraInit() != ESP_OK) haltWithError("Camera init failed");
  applySensorSettings();
  wifiConnect();
  server.begin();
  Serial.println("CAM ready");
}

void loop() {
  shutdownIfOverheated();
  announceIfDue();
  WiFiClient client = server.accept();
  if (client) handleClient(client);
  else delay(10);  // idle: let the CPU rest
}