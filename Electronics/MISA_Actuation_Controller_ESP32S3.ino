// =================================================================
//  ESP32-S3 DEVKIT  -  FINAL COMPLETE VERSION (new board, replacing
//  the one that burned out)
//  Combines: pan/tilt servos, laser (manual-only), DFPlayer alert
//  sound on target lock, TF-Luna distance sensor, and a calibrated
//  1.8" ST7735 SPI TFT dashboard.
//
//  *** PIN MAPPING STATUS ***
//  Unlike the camera board, this is a STANDARD Espressif
//  ESP32-S3-DevKitC-1 (44-pin), which has OFFICIAL published
//  documentation - not a guess. All pins below were chosen while
//  strictly avoiding, per Espressif's official pin guide:
//    - Strapping pins (boot-critical): GPIO 0, 3, 45, 46
//    - Native USB D+/D-: GPIO 19, 20
//    - Main Serial (USB-UART bridge): GPIO 43, 44
//    - Octal PSRAM internal use (N16R8 = Octal variant): GPIO 35, 36, 37
//    - Onboard RGB LED (varies by board revision): GPIO 38 or 48
//  Confidence in this pin map is HIGH, but still verify once the
//  board is in hand that it is genuinely the standard DevKitC-1
//  layout and not a relabeled clone.
//
//  SAFETY: the laser is ONLY ever set by an explicit command field
//  from the PC - nothing in this file turns it on automatically.
// =================================================================

#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// -----------------------------------------------------------------
//  Known Wi-Fi networks - order matches what the user set on the
//  camera board, for consistency between the two files.
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

const uint16_t SERVER_PORT = 9000;

WiFiServer tcpServer(SERVER_PORT);
WiFiClient pcConnection;
String connectedNetworkName = "---";

// -----------------------------------------------------------------
//  Servo pin assignment - safe general-purpose GPIOs
// -----------------------------------------------------------------
const int PAN_PIN   = 4;
const int TILT_PIN  = 5;
const int LASER_PIN = 6;

Servo panServo;
Servo tiltServo;

// -----------------------------------------------------------------
//  DFPlayer wiring - UART channel 1, safe GPIOs
// -----------------------------------------------------------------
#define DFPLAYER_RX_PIN 7    // ESP32 RX <- DFPlayer TX
#define DFPLAYER_TX_PIN 8    // ESP32 TX -> DFPlayer RX (through ~1k resistor)

HardwareSerial dfPlayerSerial(1);
DFRobotDFPlayerMini dfPlayer;
bool dfPlayerIsReady = false;

// -----------------------------------------------------------------
//  TF-Luna wiring - UART channel 2, safe GPIOs
// -----------------------------------------------------------------
#define LUNA_RX_PIN 9    // ESP32 RX <- TF-Luna TX (pin 3)
#define LUNA_TX_PIN 10   // ESP32 TX -> TF-Luna RX (pin 2)

HardwareSerial lunaSerial(2);
int lastLunaDistanceCm = -1;

// -----------------------------------------------------------------
//  TFT display wiring - ST7735S, hardware SPI on safe GPIOs.
//  Subclass exposes setColRowStart() (protected in the base
//  library) so we can apply the same offset correction learned
//  on the previous board (col=2, row=1) - the OFFSET VALUE itself
//  is a property of this specific physical screen, not the ESP32
//  board, so it should still be correct, but must be re-verified
//  visually once wired (send a solid fillScreen color and check
//  for the noisy-edge artifact seen before).
// -----------------------------------------------------------------
#define TFT_CS   11
#define TFT_RST  12
#define TFT_DC   13
#define TFT_MOSI 14
#define TFT_SCK  15

class TftWithOffset : public Adafruit_ST7735 {
public:
  TftWithOffset(SPIClass* spiBus, int8_t csPin, int8_t dcPin, int8_t rstPin)
    : Adafruit_ST7735(spiBus, csPin, dcPin, rstPin) {}

  using Adafruit_ST7735::setColRowStart;
};

TftWithOffset tft = TftWithOffset(&SPI, TFT_CS, TFT_DC, TFT_RST);

const int8_t TFT_COL_OFFSET = 2;   // Carried over from previous calibration - re-verify on this screen
const int8_t TFT_ROW_OFFSET = 1;

// -----------------------------------------------------------------
//  Safety limits - UNCHANGED from before, these describe the
//  physical turret geometry, not the electronics
// -----------------------------------------------------------------
const float PAN_MIN_ANGLE  = 20.0;
const float PAN_MAX_ANGLE  = 160.0;
const float TILT_MIN_ANGLE = 60.0;
const float TILT_MAX_ANGLE = 150.0;

// -----------------------------------------------------------------
//  Motion smoothing state
// -----------------------------------------------------------------
float currentPanAngle  = 90.0;
float currentTiltAngle = 90.0;
float targetPanAngle   = 90.0;
float targetTiltAngle  = 90.0;

const float MAX_DEGREES_PER_STEP = 4.0;

// -----------------------------------------------------------------
//  Laser + lock state
// -----------------------------------------------------------------
bool laserShouldBeOn   = false;
bool isCurrentlyLocked = false;
bool wasLockedLastLoop = false;


// -----------------------------------------------------------------
//  Function: connectToWiFi
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
        Serial.print("ESP32-S3 DevKit IP address: ");
        Serial.println(WiFi.localIP());

        connectedNetworkName = knownNetworks[i].ssid;

        if (MDNS.begin("esp32motors")) {
          Serial.println("mDNS responder started: esp32motors.local");
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
//  Function: initDFPlayer
// -----------------------------------------------------------------
void initDFPlayer() {
  dfPlayerSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);

  if (!dfPlayer.begin(dfPlayerSerial)) {
    Serial.println("DFPlayer Mini not detected - continuing without sound.");
    dfPlayerIsReady = false;
    return;
  }

  dfPlayer.volume(20);
  dfPlayerIsReady = true;
  Serial.println("DFPlayer Mini ready.");
}


// -----------------------------------------------------------------
//  Function: initLuna
// -----------------------------------------------------------------
void initLuna() {
  lunaSerial.begin(115200, SERIAL_8N1, LUNA_RX_PIN, LUNA_TX_PIN);
  Serial.println("TF-Luna serial channel opened.");
}


// -----------------------------------------------------------------
//  Function: updateLunaReading
// -----------------------------------------------------------------
void updateLunaReading() {
  if (lunaSerial.available() < 9) {
    return;
  }

  if (lunaSerial.peek() != 0x59) {
    lunaSerial.read();
    return;
  }

  uint8_t frame[9];
  lunaSerial.readBytes(frame, 9);

  if (frame[0] != 0x59 || frame[1] != 0x59) {
    return;
  }

  uint8_t checksum = 0;
  for (int i = 0; i < 8; i++) {
    checksum += frame[i];
  }
  if (checksum != frame[8]) {
    return;
  }

  lastLunaDistanceCm = frame[2] | (frame[3] << 8);
}


// -----------------------------------------------------------------
//  Function: clampAngle
// -----------------------------------------------------------------
float clampAngle(float value, float minValue, float maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}


// -----------------------------------------------------------------
//  Function: parseAndSetTarget
//  What it does: parses "pan,tilt,laser,locked" - 4 comma-separated
//  fields, matching what the Python side sends.
// -----------------------------------------------------------------
void parseAndSetTarget(String line) {
  int firstComma  = line.indexOf(',');
  int secondComma = line.indexOf(',', firstComma + 1);
  int thirdComma  = line.indexOf(',', secondComma + 1);

  if (firstComma == -1 || secondComma == -1 || thirdComma == -1) {
    Serial.println("Malformed command, ignoring: " + line);
    return;
  }

  String panText   = line.substring(0, firstComma);
  String tiltText  = line.substring(firstComma + 1, secondComma);
  String laserText = line.substring(secondComma + 1, thirdComma);
  String lockText  = line.substring(thirdComma + 1);

  float requestedPan  = panText.toFloat();
  float requestedTilt = tiltText.toFloat();
  int   requestedLaser = laserText.toInt();
  int   requestedLock  = lockText.toInt();

  targetPanAngle    = clampAngle(requestedPan,  PAN_MIN_ANGLE,  PAN_MAX_ANGLE);
  targetTiltAngle   = clampAngle(requestedTilt, TILT_MIN_ANGLE, TILT_MAX_ANGLE);
  laserShouldBeOn   = (requestedLaser == 1);
  isCurrentlyLocked = (requestedLock == 1);
}


// -----------------------------------------------------------------
//  Function: updateAlertSound
// -----------------------------------------------------------------
void updateAlertSound() {
  bool lockJustStarted = isCurrentlyLocked && !wasLockedLastLoop;

  if (lockJustStarted && dfPlayerIsReady) {
    dfPlayer.play(1);
    Serial.println("Target lock acquired - playing alert sound.");
  }

  wasLockedLastLoop = isCurrentlyLocked;
}


// -----------------------------------------------------------------
//  Function: updateServoPositions
// -----------------------------------------------------------------
void updateServoPositions() {
  float panDifference = targetPanAngle - currentPanAngle;
  if (abs(panDifference) > MAX_DEGREES_PER_STEP) {
    currentPanAngle += (panDifference > 0) ? MAX_DEGREES_PER_STEP : -MAX_DEGREES_PER_STEP;
  } else {
    currentPanAngle = targetPanAngle;
  }

  float tiltDifference = targetTiltAngle - currentTiltAngle;
  if (abs(tiltDifference) > MAX_DEGREES_PER_STEP) {
    currentTiltAngle += (tiltDifference > 0) ? MAX_DEGREES_PER_STEP : -MAX_DEGREES_PER_STEP;
  } else {
    currentTiltAngle = targetTiltAngle;
  }

  currentPanAngle  = clampAngle(currentPanAngle,  PAN_MIN_ANGLE,  PAN_MAX_ANGLE);
  currentTiltAngle = clampAngle(currentTiltAngle, TILT_MIN_ANGLE, TILT_MAX_ANGLE);

  panServo.write(currentPanAngle);
  tiltServo.write(currentTiltAngle);
}


// -----------------------------------------------------------------
//  Function: updateLaser
// -----------------------------------------------------------------
void updateLaser() {
  digitalWrite(LASER_PIN, laserShouldBeOn ? HIGH : LOW);
}


// -----------------------------------------------------------------
//  Function: watchdogCheck
// -----------------------------------------------------------------
void watchdogCheck() {
  if (!pcConnection || !pcConnection.connected()) {
    laserShouldBeOn = false;
  }
}


// -----------------------------------------------------------------
//  Function: checkWiFiStillConnected
// -----------------------------------------------------------------
void checkWiFiStillConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi connection lost - laser off, reconnecting...");
    laserShouldBeOn = false;
    digitalWrite(LASER_PIN, LOW);
    connectToWiFi();
    tcpServer.begin();
  }
}


// -----------------------------------------------------------------
//  Function: initDashboard
// -----------------------------------------------------------------
void initDashboard() {
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, HIGH);
  delay(100);
  digitalWrite(TFT_RST, LOW);
  delay(100);
  digitalWrite(TFT_RST, HIGH);
  delay(100);

  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);   // SCK, MISO unused, MOSI, SS
  SPI.setFrequency(1000000);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.setColRowStart(TFT_COL_OFFSET, TFT_ROW_OFFSET);

  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);

  tft.setCursor(2, 2);
  tft.print("MISA TURRET DASHBOARD");

  tft.drawFastHLine(0, 12, 160, ST7735_BLUE);

  tft.setCursor(2, 18);  tft.print("Net:");
  tft.setCursor(2, 30);  tft.print("Pan:");
  tft.setCursor(2, 42);  tft.print("Tilt:");
  tft.setCursor(2, 54);  tft.print("Range:");

  tft.setCursor(90, 30); tft.print("Laser:");
  tft.setCursor(90, 42); tft.print("Lock:");
  tft.setCursor(90, 54); tft.print("Sound:");
}


// -----------------------------------------------------------------
//  Function: updateDashboard
// -----------------------------------------------------------------
void updateDashboard() {
  tft.fillRect(30, 18, 60, 10, ST7735_BLACK);
  tft.setCursor(30, 18);
  tft.setTextColor(ST7735_GREEN);
  tft.print(connectedNetworkName);

  tft.fillRect(30, 30, 55, 10, ST7735_BLACK);
  tft.setCursor(30, 30);
  tft.setTextColor(ST7735_WHITE);
  tft.print(currentPanAngle, 1);

  tft.fillRect(34, 42, 55, 10, ST7735_BLACK);
  tft.setCursor(34, 42);
  tft.print(currentTiltAngle, 1);

  tft.fillRect(40, 54, 45, 10, ST7735_BLACK);
  tft.setCursor(40, 54);
  if (lastLunaDistanceCm >= 0) {
    tft.print(lastLunaDistanceCm);
    tft.print("cm");
  } else {
    tft.print("---");
  }

  tft.fillRect(128, 30, 30, 10, ST7735_BLACK);
  tft.setCursor(128, 30);
  tft.setTextColor(laserShouldBeOn ? ST7735_RED : ST7735_WHITE);
  tft.print(laserShouldBeOn ? "ON" : "off");

  tft.fillRect(122, 42, 36, 10, ST7735_BLACK);
  tft.setCursor(122, 42);
  tft.setTextColor(isCurrentlyLocked ? ST7735_YELLOW : ST7735_WHITE);
  tft.print(isCurrentlyLocked ? "LOCK" : "----");

  tft.fillRect(130, 54, 28, 10, ST7735_BLACK);
  tft.setCursor(130, 54);
  tft.setTextColor(dfPlayerIsReady ? ST7735_GREEN : ST7735_RED);
  tft.print(dfPlayerIsReady ? "OK" : "N/A");

  tft.fillRect(0, 70, 160, 10, ST7735_BLACK);
  tft.setCursor(2, 70);
  tft.setTextColor((pcConnection && pcConnection.connected()) ? ST7735_GREEN : ST7735_RED);
  tft.print((pcConnection && pcConnection.connected()) ? "PC: connected" : "PC: waiting...");
}


// =================================================================
//  MAIN PROGRAM
// =================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, LOW);

  initDashboard();

  connectToWiFi();

  initDFPlayer();
  initLuna();

  panServo.attach(PAN_PIN);
  tiltServo.attach(TILT_PIN);

  panServo.write(currentPanAngle);
  tiltServo.write(currentTiltAngle);

  tcpServer.begin();
  Serial.println("Full controller ready: motors, laser, sound, LiDAR, dashboard.");
}

void loop() {
  checkWiFiStillConnected();

  if (!pcConnection || !pcConnection.connected()) {
    pcConnection = tcpServer.available();
  }

  if (pcConnection && pcConnection.connected() && pcConnection.available() > 0) {
    String line = pcConnection.readStringUntil('\n');
    parseAndSetTarget(line);
  }

  updateLunaReading();

  watchdogCheck();
  updateServoPositions();
  updateLaser();
  updateAlertSound();

  static unsigned long lastDashboardUpdate = 0;
  if (millis() - lastDashboardUpdate > 200) {
    updateDashboard();
    lastDashboardUpdate = millis();
  }

  delay(20);
}