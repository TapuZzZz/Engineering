// ==========================================
// Settings
// ==========================================
#define SERIAL_BAUD        115200
#define CAM_XCLK_FREQ_HZ   10000000  // 10 MHz - less noise for WiFi
#define CAM_JPEG_QUALITY   12        // higher number = smaller frames = faster
#define CAM_FB_COUNT       2
#define CAM_FRAME_SIZE     FRAMESIZE_VGA

// ==========================================
// Network Settings
// ==========================================
#define WIFI_CONNECT_TIMEOUT_MS  10000
#define WIFI_REPORT_INTERVAL_MS  5000
#define WIFI_TX_POWER            WIFI_POWER_8_5dBm   // lower = more stable on weak antennas
#define MDNS_HOSTNAME            "misa-cam"
#define WEB_SERVER_PORT          80

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

#define CAM_PIN_RGB     48

// ==========================================
// Types
// ==========================================
struct WifiNetwork {
  const char* ssid;
  const char* password;
};

// ==========================================
// Serial
// ==========================================
void serialSetup() {
  Serial.begin(SERIAL_BAUD);
  Serial.printf("Reset reason: %d\n", esp_reset_reason());
  Serial.printf("PSRAM found: %s\n", psramFound() ? "YES" : "NO");
}