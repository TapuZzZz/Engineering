// ##########################################
//                 CAMERA
// ##########################################

// ==========================================
// ICamera - interface: what every camera must do
// ==========================================
class ICamera {
public:
  virtual ~ICamera() {}
  virtual bool begin() = 0;
  virtual camera_fb_t* capture() = 0;
  virtual void release(camera_fb_t* frame) = 0;
};

// ==========================================
// CameraConfig - builds config from CAM_1_Settings
// ==========================================
class CameraConfig {
public:
  camera_config_t build() const {
    camera_config_t config = {};
    setDataPins(config);
    setControlPins(config);
    setClock(config);
    setImage(config);
    return config;
  }

private:
  void setDataPins(camera_config_t& config) const {
    config.pin_d0 = CAM_PIN_D0;
    config.pin_d1 = CAM_PIN_D1;
    config.pin_d2 = CAM_PIN_D2;
    config.pin_d3 = CAM_PIN_D3;
    config.pin_d4 = CAM_PIN_D4;
    config.pin_d5 = CAM_PIN_D5;
    config.pin_d6 = CAM_PIN_D6;
    config.pin_d7 = CAM_PIN_D7;
  }

  void setControlPins(camera_config_t& config) const {
    config.pin_xclk     = CAM_PIN_XCLK;
    config.pin_pclk     = CAM_PIN_PCLK;
    config.pin_vsync    = CAM_PIN_VSYNC;
    config.pin_href     = CAM_PIN_HREF;
    config.pin_sccb_sda = CAM_PIN_SIOD;
    config.pin_sccb_scl = CAM_PIN_SIOC;
    config.pin_pwdn     = CAM_PIN_PWDN;
    config.pin_reset    = CAM_PIN_RESET;
  }

  void setClock(camera_config_t& config) const {
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;
    config.xclk_freq_hz = CAM_XCLK_FREQ_HZ;
  }

  void setImage(camera_config_t& config) const {
    config.frame_size   = CAM_FRAME_SIZE;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode    = CAMERA_GRAB_LATEST;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = CAM_JPEG_QUALITY;
    config.fb_count     = CAM_FB_COUNT;
  }
};

// ==========================================
// Esp32Camera - ICamera implementation using esp_camera
// ==========================================
class Esp32Camera : public ICamera {
public:
  Esp32Camera(const CameraConfig& config) : _config(config) {}

  bool begin() override {
    camera_config_t config = _config.build();
    return esp_camera_init(&config) == ESP_OK;
  }

  camera_fb_t* capture() override {
    return esp_camera_fb_get();
  }

  void release(camera_fb_t* frame) override {
    esp_camera_fb_return(frame);
  }

private:
  const CameraConfig& _config;
};

// ==========================================
// FpsCounter - counts frames and prints FPS every second
// ==========================================
class FpsCounter {
public:
  FpsCounter() : _frames(0), _lastTime(0) {}

  void start() {
    _frames = 0;
    _lastTime = millis();
  }

  void tick() {
    _frames++;
    if (millis() - _lastTime >= 1000) {
      Serial.printf("Stream FPS: %d\n", _frames);
      start();
    }
  }

private:
  int _frames;
  unsigned long _lastTime;
};

// ##########################################
//                 NETWORK
// ##########################################

// ==========================================
// INetwork - interface: what every network must do
// ==========================================
class INetwork {
public:
  virtual ~INetwork() {}
  virtual bool connect() = 0;
  virtual bool isConnected() = 0;
  virtual String localIp() = 0;
  virtual int signalStrength() = 0;
};

// ==========================================
// WifiConnector - tries each network from CAM_2
// ==========================================
class WifiConnector : public INetwork {
public:
  WifiConnector(const WifiNetwork* networks, int count)
    : _networks(networks), _count(count) {}

  bool connect() override {
    WiFi.mode(WIFI_STA);
    for (int i = 0; i < _count; i++) {
      if (tryNetwork(_networks[i])) {
        disablePowerSave();
        return true;
      }
    }
    return false;
  }

  bool isConnected() override {
    return WiFi.status() == WL_CONNECTED;
  }

  String localIp() override {
    return WiFi.localIP().toString();
  }

  int signalStrength() override {
    return WiFi.RSSI();
  }

private:
  bool tryNetwork(const WifiNetwork& network) {
    Serial.printf("Connecting to %s...\n", network.ssid);
    WiFi.begin(network.ssid, network.password);
    limitTxPower();
    return waitForConnection();
  }

  bool waitForConnection() {
    unsigned long start = millis();
    while (millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
      if (isConnected()) {
        return true;
      }
      delay(100);
    }
    WiFi.disconnect();
    return false;
  }

  void limitTxPower() {
    WiFi.setTxPower(WIFI_TX_POWER);
  }

  void disablePowerSave() {
    WiFi.setSleep(false);
  }

  const WifiNetwork* _networks;
  int _count;
};

// ==========================================
// SignalReporter - prints WiFi strength every few seconds
// ==========================================
class SignalReporter {
public:
  SignalReporter(INetwork& network) : _network(network), _lastTime(0) {}

  void update() {
    if (millis() - _lastTime >= WIFI_REPORT_INTERVAL_MS) {
      _lastTime = millis();
      report();
    }
  }

private:
  void report() {
    int rssi = _network.signalStrength();
    Serial.printf("WiFi RSSI: %d dBm (%s)\n", rssi, qualityLabel(rssi));
  }

  const char* qualityLabel(int rssi) {
    if (rssi >= -60) return "Excellent";
    if (rssi >= -70) return "Good";
    if (rssi >= -80) return "Weak";
    return "Bad";
  }

  INetwork& _network;
  unsigned long _lastTime;
};

// ==========================================
// MdnsService - gives the board a name on the network
// ==========================================
class MdnsService {
public:
  bool begin(const char* hostname) {
    return MDNS.begin(hostname);
  }
};

// ##########################################
//                 OBJECTS
// ##########################################
CameraConfig   cameraConfig;
Esp32Camera    esp32Camera(cameraConfig);
ICamera&       camera = esp32Camera;

WifiConnector  wifiConnector(WIFI_NETWORKS, WIFI_NETWORKS_COUNT);
INetwork&      network = wifiConnector;
SignalReporter signalReporter(network);
MdnsService    mdns;

// ##########################################
//          CALLED FROM MISA_Cam.ino
// ##########################################
void cameraSetup() {
  if (camera.begin()) {
    Serial.println("Camera init OK");
  } else {
    Serial.println("Camera init FAILED");
  }
}

void networkSetup() {
  if (network.connect()) {
    Serial.println("WiFi OK, IP: " + network.localIp());
  } else {
    Serial.println("WiFi FAILED");
  }
}

void mdnsSetup() {
  if (mdns.begin(MDNS_HOSTNAME)) {
    Serial.println("mDNS: http://" MDNS_HOSTNAME ".local");
  } else {
    Serial.println("mDNS FAILED");
  }
}

void networkMonitor() {
  signalReporter.update();
}