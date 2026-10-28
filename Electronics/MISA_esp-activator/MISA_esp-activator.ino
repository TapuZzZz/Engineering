#include <WiFi.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <ESP32Servo.h>
#include <DFRobotDFPlayerMini.h>

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

const char* HOSTNAME = "esp-activator";
const char* AP_SSID  = "ESP-Activator";  // own network if all WiFi fails
const char* AP_PASS  = "03032007";

#define WIFI_TIMEOUT_MS 10000
#define UDP_PORT        4210   // tracking messages from the Mac
#define DISCOVERY_PORT  5005   // HELLO announcements to the Mac
#define ANNOUNCE_MS     1000
#define LINK_TIMEOUT_MS 1500   // no message this long = Mac not connected

// ===================== Pins (KiCad schematic) =====================
#define TFT_CS        4
#define TFT_DC        5
#define TFT_RST       6
#define TFT_MOSI      7    // "SDA" on the display
#define TFT_SCLK      15   // "SCL" on the display

#define LIDAR_ESP_RX  16   // TF-Luna TX -> here
#define LIDAR_ESP_TX  17   // here -> TF-Luna RX

#define PAN_PIN       18
#define TILT_PIN      10
#define LASER_PIN     11   // drives a transistor / MOSFET, HIGH = on

#define MP3_ESP_TX    12   // here -> DFPlayer RX (through 1k)
#define MP3_ESP_RX    13   // DFPlayer TX -> here
#define MP3_BUSY_PIN  14   // LOW while playing

#define RGB_PIN       RGB_BUILTIN  // change to 38 on a DevKitC-1 v1.1

// ===================== Servo settings =====================
#define PAN_MIN      20
#define PAN_MAX      160
#define TILT_MIN     60
#define TILT_MAX     145
#define HOME_ANGLE   90
#define SCAN_TILT    90

#define PAN_DIR      1   // directions are fixed in Python
#define TILT_DIR     1

#define DEAD_ZONE    8       // offsets below this % are ignored (no jitter)
#define GAIN         0.015f  // degrees per % offset, per message
#define MAX_STEP     1.0f    // max degrees added per message (no overshoot)
#define STEP_MS      15      // tracking speed: ms per 1-degree step
#define SLOW_STEP_MS 40      // scanning / going home: gentle

// ===================== Lock / laser settings =====================
#define LOCK_ZONE        10     // % from the laser dot that counts as "on target"
#define LOCK_CONFIRM_MS  500    // must stay on target this long to lock
#define LOST_TIMEOUT_MS  3000   // tracking: no target this long -> scan
#define LOCK_LOST_MS     500    // locked: target lost -> laser off at once
#define LASER_MAX_MS     10000  // laser never on longer than this
#define LASER_REST_MS    10000  // laser cool-down after every use

// ===================== Laser parallax calibration =====================
// Where the laser dot appears in the image (same units as dx/dy: % from center),
// measured at two distances. Click the green dot in the Python window to read X / Y.
#define CAL_NEAR_CM     50
#define CAL_NEAR_X      0
#define CAL_NEAR_Y      0
#define CAL_FAR_CM      200
#define CAL_FAR_X       0
#define CAL_FAR_Y       0
#define CAL_DEFAULT_CM  100   // used when the lidar has no reading

// ===================== Lidar / sound / safety settings =====================
#define LIDAR_MIN_CM     20
#define LIDAR_MAX_CM     300
#define LIDAR_MIN_AMP    100    // weaker signal = unreliable reading
#define LIDAR_STALE_MS   200

#define MP3_VOLUME       20     // 0..30
#define TRACK_LOCKED     1      // 0001.mp3 = "target locked"

#define MAX_CHIP_TEMP    75.0f  // °C - shut down above this
#define TEMP_CHECK_MS    2000

// ===================== Display / LED settings =====================
#define DISPLAY_MS    150
#define LED_MS        20
#define VIEW_DY_SIGN  -1   // Python inverts tilt (INVERT_Y) - flip back for drawing

#define SCREEN_W   160
#define SCREEN_H   128
#define HEADER_H   14
#define RADAR_CX   80
#define RADAR_CY   124
#define RADAR_R    76
#define MAX_BLIPS  30
#define VF_X       6     // tracking viewfinder
#define VF_Y       20
#define VF_W       64
#define VF_H       100
#define TX         80    // tracking text column
#define BAR_X      80    // lock progress bar
#define BAR_Y      108
#define BAR_W      74
#define BAR_H      8

// ===================== Colors =====================
#define COLOR_BG            ST77XX_BLACK   // screen background - always black
#define COLOR_DARK          0x18E3         // empty part of the lock bar
#define COLOR_GRID          0x022B
#define COLOR_DARK_RED      0x7800
#define COLOR_PASTEL_YELLOW 0xFFF6
#define COLOR_PASTEL_GREEN  0xB7F6
#define COLOR_PASTEL_PINK   0xFDB8

// ===================== States =====================
enum State { IDLE, SCANNING, TRACKING, LOCKED };

struct StateInfo {
  const char* name;
  uint16_t    color;
};

const StateInfo STATE_INFO[] = {
  { "STANDBY",  COLOR_PASTEL_PINK   },
  { "SCANNING", ST77XX_CYAN         },
  { "TRACKING", COLOR_PASTEL_YELLOW },
  { "LOCKED",   ST77XX_RED          },
};

// ===================== Objects =====================
WiFiUDP             udp;
Servo               panServo;
Servo               tiltServo;
Adafruit_ST7735     tft(&SPI, TFT_CS, TFT_DC, TFT_RST);
HardwareSerial      lidarSerial(1);
HardwareSerial      mp3Serial(2);
DFRobotDFPlayerMini player;

// ===================== State variables =====================
State state = IDLE;
unsigned long stateStart = 0;
bool screenDirty = true;

float targetPan   = HOME_ANGLE;
float targetTilt  = HOME_ANGLE;
int   currentPan  = HOME_ANGLE;
int   currentTilt = HOME_ANGLE;
unsigned long lastStepTime = 0;

int lastDx = 100;   // target -> laser dot, % (after parallax correction)
int lastDy = 100;
unsigned long lastSeenTime    = 0;
unsigned long lastMessageTime = 0;
unsigned long centeredSince   = 0;

bool laserIsOn = false;
unsigned long laserOnSince = 0;
unsigned long laserReadyAt = 0;

uint8_t lidarFrame[9];
int lidarIndex    = 0;
int lidarDistance = -1;
unsigned long lidarTime = 0;

struct Blip {
  int angle;
  int radius;
};
Blip blips[MAX_BLIPS];
int blipCount = 0;
int blipNext  = 0;

bool playerReady = false;

// ===================== Laser =====================
void laserOn() {
  digitalWrite(LASER_PIN, HIGH);
  laserIsOn = true;
  laserOnSince = millis();
}

void laserOff() {
  digitalWrite(LASER_PIN, LOW);
  laserIsOn = false;
}

void laserSafetyCheck() {
  if (laserIsOn && millis() - laserOnSince > LASER_MAX_MS) laserOff();
}

unsigned long laserSecondsLeft() {
  unsigned long elapsed = millis() - stateStart;
  if (elapsed >= LASER_MAX_MS) return 0;
  return (LASER_MAX_MS - elapsed + 999) / 1000;
}

// ===================== Parallax correction =====================
// Laser dot position in the image at this distance: A + B / distance
float spotOffset(float nearValue, float farValue, float distanceCm) {
  float b = (nearValue - farValue) / (1.0f / CAL_NEAR_CM - 1.0f / CAL_FAR_CM);
  float a = nearValue - b / CAL_NEAR_CM;
  return a + b / distanceCm;
}

float aimDistance() {
  return lidarHasTarget() ? lidarDistance : CAL_DEFAULT_CM;
}

// ===================== RGB LED effects =====================
float breath(unsigned long periodMs) {  // smooth 0..1..0
  float phase = (millis() % periodMs) / (float)periodMs;
  return 0.5f - 0.5f * cosf(phase * 2 * PI);
}

bool blinkOn(unsigned long periodMs) {
  return (millis() % periodMs) < periodMs / 2;
}

void ledScaled(uint8_t r, uint8_t g, uint8_t b, float k) {
  rgbLedWrite(RGB_PIN, r * k, g * k, b * k);
}

void updateLed() {
  static unsigned long lastTime = 0;
  if (millis() - lastTime < LED_MS) return;
  lastTime = millis();
  switch (state) {
    case IDLE:     ledScaled(30, 0, 60, breath(3000));                 break;
    case SCANNING: ledScaled(0, 50, 60, breath(1200));                 break;
    case TRACKING: ledScaled(70, 50, 0, 0.3f + 0.7f * breath(400));    break;
    case LOCKED:   ledScaled(90, 0, 0, blinkOn(200) ? 1.0f : 0.05f);   break;
  }
}

// ===================== Display helpers =====================
void printAt(int x, int y, uint8_t size, uint16_t fg, uint16_t bg, const char* text, int width) {
  char padded[28];
  snprintf(padded, sizeof(padded), "%-*s", width, text);  // spaces erase old text
  tft.setTextSize(size);
  tft.setTextColor(fg, bg);
  tft.setCursor(x, y);
  tft.print(padded);
}

void printRow(int x, int y, uint16_t color, const char* format, int value) {
  char text[20];
  snprintf(text, sizeof(text), format, value);
  printAt(x, y, 1, color, COLOR_BG, text, 13);
}

void formatDistance(char* text, size_t size) {
  if (lidarFresh()) snprintf(text, size, "%dcm", lidarDistance);
  else snprintf(text, size, "---");
}

void laserText(char* text, size_t size) {
  unsigned long now = millis();
  if (laserIsOn) snprintf(text, size, "LASER ON");
  else if (now < laserReadyAt) snprintf(text, size, "REST %lus", (laserReadyAt - now) / 1000 + 1);
  else snprintf(text, size, "LSR RDY");
}

// ===================== Header (all screens) =====================
void drawHeader() {
  uint16_t color = STATE_INFO[state].color;
  tft.fillRect(0, 0, SCREEN_W, HEADER_H, color);
  printAt(4, 4, 1, COLOR_BG, color, STATE_INFO[state].name, 0);
}

void drawHeaderLaser() {
  char text[12];
  laserText(text, sizeof(text));
  printAt(100, 4, 1, COLOR_BG, STATE_INFO[state].color, text, 9);
}

// ===================== Screen: STANDBY =====================
void drawIdleStatic() {
  tft.fillScreen(COLOR_BG);
  drawHeader();
  printAt(2, 22, 2, ST77XX_CYAN, COLOR_BG, "ESP-ACTIVATOR", 0);
  tft.drawFastHLine(2, 42, 156, ST77XX_RED);
  printAt(8, 46, 1, COLOR_PASTEL_PINK, COLOR_BG, "Home 90/90 - laser off", 0);
}

void drawNetworkRow(int x, int y) {
  bool ap = WiFi.getMode() == WIFI_AP;
  String text = (ap ? "AP " : "IP ") + (ap ? WiFi.softAPIP() : WiFi.localIP()).toString();
  printAt(x, y, 1, COLOR_PASTEL_PINK, COLOR_BG, text.c_str(), 20);
}

void drawSensorRow(int x, int y) {
  char dist[10];
  char text[26];
  formatDistance(dist, sizeof(dist));
  snprintf(text, sizeof(text), "CHIP %.0fC   DIST %s", temperatureRead(), dist);
  printAt(x, y, 1, ST77XX_CYAN, COLOR_BG, text, 24);
}

void drawIdleDynamic() {
  char text[24];
  int dots = (millis() / 400) % 4;
  snprintf(text, sizeof(text), "Waiting for Mac%.*s", dots, "...");
  printAt(8, 64, 1, COLOR_PASTEL_YELLOW, COLOR_BG, text, 19);
  drawNetworkRow(8, 84);
  drawSensorRow(8, 98);
  drawHeaderLaser();
}

// ===================== Screen: SCANNING (radar) =====================
int radarX(float angle, float r) {
  return RADAR_CX + lroundf(r * cosf(angle * DEG_TO_RAD));
}

int radarY(float angle, float r) {
  return RADAR_CY - lroundf(r * sinf(angle * DEG_TO_RAD));
}

void drawArcDots(int r, uint16_t color) {
  for (int a = 0; a <= 180; a += 3) tft.drawPixel(radarX(a, r), radarY(a, r), color);
}

void drawSweep(int angle, uint16_t color) {
  tft.drawLine(RADAR_CX, RADAR_CY, radarX(angle, RADAR_R), radarY(angle, RADAR_R), color);
}

void drawRadarGrid() {
  drawArcDots(RADAR_R / 3, COLOR_GRID);
  drawArcDots(RADAR_R * 2 / 3, COLOR_GRID);
  drawArcDots(RADAR_R, ST77XX_CYAN);
  tft.drawFastHLine(RADAR_CX - RADAR_R, RADAR_CY, RADAR_R * 2, COLOR_GRID);
  drawSweep(PAN_MIN, COLOR_DARK_RED);
  drawSweep(PAN_MAX, COLOR_DARK_RED);
}

void drawBlip(const Blip& blip, uint16_t color) {
  tft.fillCircle(radarX(blip.angle, blip.radius), radarY(blip.angle, blip.radius), 2, color);
}

void addBlipIfAny() {
  if (!lidarHasTarget()) return;
  int r = map(lidarDistance, 0, LIDAR_MAX_CM, 0, RADAR_R);
  if (blipCount == MAX_BLIPS) drawBlip(blips[blipNext], COLOR_BG);  // erase oldest
  blips[blipNext] = { currentPan, r };
  blipNext = (blipNext + 1) % MAX_BLIPS;
  if (blipCount < MAX_BLIPS) blipCount++;
}

void drawBlips() {
  for (int i = 0; i < blipCount; i++) drawBlip(blips[i], COLOR_PASTEL_GREEN);
}

void drawRadarText() {
  char dist[10];
  char text[20];
  formatDistance(dist, sizeof(dist));
  snprintf(text, sizeof(text), "DIST %s", dist);
  printAt(4, 18, 1, ST77XX_CYAN, COLOR_BG, text, 12);
  printRow(104, 18, COLOR_PASTEL_YELLOW, "PAN %3d", currentPan);
  printRow(104, 30, COLOR_PASTEL_YELLOW, "TLT %3d", currentTilt);
}

void drawRadarStatic() {
  tft.fillScreen(COLOR_BG);
  drawHeader();
  blipCount = 0;
  blipNext  = 0;
}

void drawRadarDynamic() {
  static int lastAngle = HOME_ANGLE;
  drawSweep(lastAngle, COLOR_BG);  // erase previous sweep line
  drawRadarGrid();
  addBlipIfAny();
  drawBlips();
  drawSweep(currentPan, ST77XX_CYAN);
  lastAngle = currentPan;
  drawRadarText();
  drawHeaderLaser();
}

// ===================== Screen: TRACKING (viewfinder) =====================
// The cross in the middle = where the laser dot is
void drawViewfinder() {
  int cx = VF_X + VF_W / 2;
  int cy = VF_Y + VF_H / 2;
  int zx = VF_W * LOCK_ZONE / 200;  // lock zone half size
  int zy = VF_H * LOCK_ZONE / 200;
  tft.drawRect(VF_X, VF_Y, VF_W, VF_H, ST77XX_CYAN);
  tft.drawFastHLine(VF_X + 1, cy, VF_W - 2, COLOR_GRID);
  tft.drawFastVLine(cx, VF_Y + 1, VF_H - 2, COLOR_GRID);
  tft.drawRect(cx - zx, cy - zy, zx * 2 + 1, zy * 2 + 1, COLOR_PASTEL_GREEN);
}

void drawTargetMarker(int dx, int dy, uint16_t color) {
  int x = VF_X + VF_W / 2 + dx * (VF_W / 2 - 6) / 100;
  int y = VF_Y + VF_H / 2 + dy * (VF_H / 2 - 6) / 100;
  tft.drawCircle(x, y, 5, color);
  tft.drawCircle(x, y, 2, color);
}

void drawTrackingText() {
  char dist[10];
  char text[20];
  formatDistance(dist, sizeof(dist));
  printRow(TX, 20, ST77XX_CYAN, "dx   %+4d", lastDx);
  printRow(TX, 32, ST77XX_CYAN, "dy   %+4d", lastDy * VIEW_DY_SIGN);
  printRow(TX, 50, COLOR_PASTEL_YELLOW, "PAN  %4d", currentPan);
  printRow(TX, 62, COLOR_PASTEL_YELLOW, "TILT %4d", currentTilt);
  snprintf(text, sizeof(text), "DIST %s", dist);
  printAt(TX, 80, 1, ST77XX_CYAN, COLOR_BG, text, 13);
}

void drawLockBar() {
  float progress = 0;
  if (centeredSince > 0) progress = (millis() - centeredSince) / (float)LOCK_CONFIRM_MS;
  int filled = constrain(progress, 0.0f, 1.0f) * BAR_W;
  tft.fillRect(BAR_X, BAR_Y, filled, BAR_H, ST77XX_RED);
  tft.fillRect(BAR_X + filled, BAR_Y, BAR_W - filled, BAR_H, COLOR_DARK);
}

void drawTrackingStatic() {
  tft.fillScreen(COLOR_BG);
  drawHeader();
  printAt(BAR_X, BAR_Y - 11, 1, COLOR_PASTEL_PINK, COLOR_BG, "LOCK", 0);
}

void drawTrackingDynamic() {
  static int markerX = 0;
  static int markerY = 0;
  drawTargetMarker(markerX, markerY, COLOR_BG);  // erase old marker
  drawViewfinder();
  markerX = constrain(lastDx, -100, 100);
  markerY = constrain(lastDy * VIEW_DY_SIGN, -100, 100);
  drawTargetMarker(markerX, markerY, ST77XX_RED);
  drawTrackingText();
  drawLockBar();
  drawHeaderLaser();
}

// ===================== Screen: LOCKED =====================
void drawLockedStatic() {
  tft.fillScreen(COLOR_BG);
  drawHeader();
  printAt(2, 22, 2, ST77XX_RED, COLOR_BG, "TARGET LOCKED", 0);
  printAt(108, 74, 1, COLOR_PASTEL_PINK, COLOR_BG, "sec", 0);
}

void drawLockedBorder() {
  uint16_t color = blinkOn(400) ? ST77XX_RED : COLOR_BG;
  tft.drawRect(0, HEADER_H, SCREEN_W, SCREEN_H - HEADER_H, color);
  tft.drawRect(1, HEADER_H + 1, SCREEN_W - 2, SCREEN_H - HEADER_H - 2, color);
}

void drawLockedDynamic() {
  char text[12];
  snprintf(text, sizeof(text), "%2lu", laserSecondsLeft());
  printAt(44, 44, 5, COLOR_PASTEL_YELLOW, COLOR_BG, text, 0);
  formatDistance(text, sizeof(text));
  printAt(44, 96, 2, ST77XX_WHITE, COLOR_BG, text, 7);
  drawLockedBorder();
  drawHeaderLaser();
}

// ===================== Screen dispatch =====================
void drawScreenStatic() {
  switch (state) {
    case IDLE:     drawIdleStatic();     break;
    case SCANNING: drawRadarStatic();    break;
    case TRACKING: drawTrackingStatic(); break;
    case LOCKED:   drawLockedStatic();   break;
  }
}

void drawScreenDynamic() {
  switch (state) {
    case IDLE:     drawIdleDynamic();     break;
    case SCANNING: drawRadarDynamic();    break;
    case TRACKING: drawTrackingDynamic(); break;
    case LOCKED:   drawLockedDynamic();   break;
  }
}

void updateDisplay() {
  static unsigned long lastTime = 0;
  if (screenDirty) {
    screenDirty = false;
    drawScreenStatic();
  }
  if (millis() - lastTime < DISPLAY_MS) return;
  lastTime = millis();
  drawScreenDynamic();
}

void displayInit() {
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.initR(INITR_BLACKTAB);
  tft.setSPISpeed(40000000);
  tft.setRotation(3);  // landscape, flipped 180
  tft.setTextWrap(false);
  tft.fillScreen(COLOR_BG);
  printAt(2, 40, 2, ST77XX_CYAN, COLOR_BG, "ESP-ACTIVATOR", 0);
  printAt(8, 70, 1, COLOR_PASTEL_PINK, COLOR_BG, "Booting...", 20);
}

// ===================== Overheat protection =====================
void showOverheat(float temp) {
  char text[20];
  tft.fillScreen(COLOR_BG);
  printAt(16, 40, 2, ST77XX_RED, COLOR_BG, "OVERHEAT", 0);
  snprintf(text, sizeof(text), "%.1f C - OFF", temp);
  printAt(16, 70, 1, COLOR_PASTEL_YELLOW, COLOR_BG, text, 0);
}

void shutdownIfOverheated() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < TEMP_CHECK_MS) return;
  lastCheck = millis();
  float temp = temperatureRead();
  if (temp < MAX_CHIP_TEMP) return;
  laserOff();
  rgbLedWrite(RGB_PIN, 0, 0, 0);
  showOverheat(temp);
  esp_deep_sleep_start();  // no wake source = stays off until RESET
}

// ===================== Sound (DFPlayer Mini) =====================
void playerInit() {
  mp3Serial.begin(9600, SERIAL_8N1, MP3_ESP_RX, MP3_ESP_TX);
  playerReady = player.begin(mp3Serial, true, false);  // no reset = faster boot
  if (!playerReady) {
    Serial.println("DFPlayer not found - check wiring / SD card");
    return;
  }
  player.volume(MP3_VOLUME);
}

void playLockSound() {
  if (!playerReady) return;
  if (digitalRead(MP3_BUSY_PIN) == LOW) return;  // already playing
  player.play(TRACK_LOCKED);
}

// ===================== Lidar (TF-Luna) =====================
void lidarInit() {
  lidarSerial.begin(115200, SERIAL_8N1, LIDAR_ESP_RX, LIDAR_ESP_TX);
}

// Collects bytes into a 9-byte frame that starts with 0x59 0x59
bool lidarAddByte(uint8_t b) {
  if (lidarIndex < 2 && b != 0x59) {
    lidarIndex = 0;
    return false;
  }
  lidarFrame[lidarIndex++] = b;
  if (lidarIndex < 9) return false;
  lidarIndex = 0;
  return true;
}

bool lidarChecksumOk() {
  uint8_t sum = 0;
  for (int i = 0; i < 8; i++) sum += lidarFrame[i];
  return sum == lidarFrame[8];
}

void decodeLidarFrame() {
  if (!lidarChecksumOk()) return;
  int distance = lidarFrame[2] | (lidarFrame[3] << 8);
  int strength = lidarFrame[4] | (lidarFrame[5] << 8);
  bool reliable = strength >= LIDAR_MIN_AMP && strength != 65535;
  lidarDistance = reliable ? distance : -1;
  lidarTime = millis();
}

void readLidar() {
  while (lidarSerial.available()) {
    if (lidarAddByte(lidarSerial.read())) decodeLidarFrame();
  }
}

bool lidarFresh() {
  return lidarDistance > 0 && millis() - lidarTime < LIDAR_STALE_MS;
}

bool lidarHasTarget() {
  return lidarFresh() && lidarDistance >= LIDAR_MIN_CM && lidarDistance <= LIDAR_MAX_CM;
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
  Serial.printf("AP mode: join %s\n", AP_SSID);
}

void wifiConnect() {
  printAt(8, 70, 1, COLOR_PASTEL_PINK, COLOR_BG, "Connecting WiFi...", 20);
  rgbLedWrite(RGB_PIN, 20, 0, 20);  // purple = connecting
  WiFi.setHostname(HOSTNAME);       // must be before mode()
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);             // no power saving = faster replies
  if (!connectToKnownNetworks()) startAccessPoint();
  udp.begin(UDP_PORT);
}

// ===================== Discovery =====================
void announceIfDue() {
  static unsigned long lastTime = 0;
  if (millis() - lastTime < ANNOUNCE_MS) return;
  lastTime = millis();
  udp.beginPacket(WiFi.broadcastIP(), DISCOVERY_PORT);
  udp.print("HELLO,ACTIVATOR");
  udp.endPacket();
}

// ===================== Servos =====================
void attachServo(Servo& servo, int pin) {
  servo.setPeriodHertz(50);
  servo.attach(pin, 500, 2400);
  servo.write(HOME_ANGLE);
  delay(500);  // one servo at a time = no current spike
}

int stepToward(int current, float target) {
  int goal = lroundf(target);
  if (current < goal) return current + 1;
  if (current > goal) return current - 1;
  return current;
}

void moveServosOneStep() {
  bool fast = (state == TRACKING || state == LOCKED);
  if (millis() - lastStepTime < (unsigned long)(fast ? STEP_MS : SLOW_STEP_MS)) return;
  lastStepTime = millis();
  currentPan  = stepToward(currentPan, targetPan);
  currentTilt = stepToward(currentTilt, targetTilt);
  panServo.write(currentPan);
  tiltServo.write(currentTilt);
}

float applyOffset(float angle, int offset, int dir, int minAngle, int maxAngle) {
  if (abs(offset) < DEAD_ZONE) return angle;
  float delta = constrain(offset * GAIN * dir, -MAX_STEP, MAX_STEP);
  return constrain(angle + delta, minAngle, maxAngle);
}

// ===================== State machine =====================
void setState(State newState) {
  state = newState;
  stateStart = millis();
  screenDirty = true;
  Serial.printf("State: %s\n", STATE_INFO[state].name);
}

void startTracking() {
  targetPan  = currentPan;   // stop scanning where it is
  targetTilt = currentTilt;
  setState(TRACKING);
}

void enterLocked() {
  setState(LOCKED);
  laserOn();
  playLockSound();
}

void exitLocked() {
  laserOff();
  laserReadyAt = millis() + LASER_REST_MS;
  setState(SCANNING);
}

bool linkAlive() {
  return lastMessageTime > 0 && millis() - lastMessageTime < LINK_TIMEOUT_MS;
}

bool targetLostFor(unsigned long ms) {
  return millis() - lastSeenTime > ms;
}

bool isCentered() {
  return abs(lastDx) < LOCK_ZONE && abs(lastDy) < LOCK_ZONE;
}

bool readyToLock() {
  if (millis() < laserReadyAt) return false;  // laser still resting
  if (!isCentered() || !lidarHasTarget()) {
    centeredSince = 0;
    return false;
  }
  if (centeredSince == 0) centeredSince = millis();
  return millis() - centeredSince >= LOCK_CONFIRM_MS;
}

void scanStep() {
  targetTilt = SCAN_TILT;
  if (currentPan != lroundf(targetPan)) return;
  targetPan = (targetPan <= PAN_MIN) ? PAN_MAX : PAN_MIN;
}

void updateIdle() {
  targetPan  = HOME_ANGLE;
  targetTilt = HOME_ANGLE;
  if (linkAlive()) setState(SCANNING);
}

void updateScanning() {
  if (!linkAlive()) {
    setState(IDLE);
    return;
  }
  scanStep();
}

void updateTracking() {
  if (!linkAlive()) setState(IDLE);
  else if (targetLostFor(LOST_TIMEOUT_MS)) setState(SCANNING);
  else if (readyToLock()) enterLocked();
}

void updateLocked() {
  bool timeUp = millis() - stateStart >= LASER_MAX_MS;
  if (timeUp || targetLostFor(LOCK_LOST_MS) || !linkAlive()) exitLocked();
}

void updateState() {
  switch (state) {
    case IDLE:     updateIdle();     break;
    case SCANNING: updateScanning(); break;
    case TRACKING: updateTracking(); break;
    case LOCKED:   updateLocked();   break;
  }
}

// ===================== UDP messages from the Mac =====================
// "T,dx,dy" = target found (offset from image center in %)
// "N"       = no target (still counts as "Mac connected")
void onTarget(int dx, int dy) {
  if (state == IDLE || state == SCANNING) startTracking();
  float d = aimDistance();
  lastDx = dx - lroundf(spotOffset(CAL_NEAR_X, CAL_FAR_X, d));  // target -> laser dot
  lastDy = dy - lroundf(spotOffset(CAL_NEAR_Y, CAL_FAR_Y, d));
  lastSeenTime = millis();
  targetPan  = applyOffset(targetPan,  lastDx, PAN_DIR,  PAN_MIN,  PAN_MAX);
  targetTilt = applyOffset(targetTilt, lastDy, TILT_DIR, TILT_MIN, TILT_MAX);
}

void handleMessage(const char* msg) {
  int dx, dy;
  if (sscanf(msg, "T,%d,%d", &dx, &dy) == 2) onTarget(dx, dy);
}

void readUdp() {
  if (udp.parsePacket() == 0) return;
  char msg[32];
  int len = udp.read(msg, sizeof(msg) - 1);
  if (len < 0) return;
  msg[len] = '\0';
  lastMessageTime = millis();
  handleMessage(msg);
}

// ===================== Main =====================
void setupPins() {
  pinMode(LASER_PIN, OUTPUT);
  laserOff();  // safety first: laser off before anything else
  pinMode(MP3_BUSY_PIN, INPUT_PULLUP);
}

void setup() {
  setupPins();
  Serial.begin(115200);
  displayInit();
  attachServo(panServo, PAN_PIN);
  attachServo(tiltServo, TILT_PIN);
  lidarInit();
  playerInit();
  wifiConnect();
  setState(IDLE);
}

void loop() {
  shutdownIfOverheated();
  announceIfDue();
  readUdp();
  readLidar();
  updateState();
  laserSafetyCheck();
  moveServosOneStep();
  updateDisplay();
  updateLed();
}