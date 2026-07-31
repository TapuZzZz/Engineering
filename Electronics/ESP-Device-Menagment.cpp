// =================================================================
//  ESP32 DEVKIT  -  FINAL COMPLETE VERSION
//  Combines: pan/tilt servos, laser (manual-only), DFPlayer alert
//  sound on target lock, and TF-Luna distance sensor.
//  Wi-Fi: tries a list of known networks forever until connected.
//  Advertised via mDNS as "esp32motors.local".
//
//  SAFETY: the laser is ONLY ever set by an explicit command field
//  from the PC - nothing in this file turns it on automatically.
// =================================================================

#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// -----------------------------------------------------------------
//  Known Wi-Fi networks - add more rows here as needed.
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

const uint16_t SERVER_PORT = 9000;

WiFiServer tcpServer(SERVER_PORT);
WiFiClient pcConnection;

// -----------------------------------------------------------------
//  Servo pin assignment
// -----------------------------------------------------------------
const int PAN_PIN   = 14;
const int TILT_PIN  = 13;
const int LASER_PIN = 26;

Servo panServo;
Servo tiltServo;

// -----------------------------------------------------------------
//  DFPlayer wiring - UART channel 1
// -----------------------------------------------------------------
#define DFPLAYER_RX_PIN 17   // ESP32 RX <- DFPlayer TX
#define DFPLAYER_TX_PIN 16   // ESP32 TX -> DFPlayer RX (through ~1k resistor)

HardwareSerial dfPlayerSerial(1);
DFRobotDFPlayerMini dfPlayer;
bool dfPlayerIsReady = false;

// -----------------------------------------------------------------
//  TF-Luna wiring - UART channel 2
//  Confirmed working: pin 2 (Luna RX) -> GPIO 25, pin 3 (Luna TX) -> GPIO 33
// -----------------------------------------------------------------
#define LUNA_RX_PIN 33   // ESP32 RX <- TF-Luna TX (pin 3)
#define LUNA_TX_PIN 25   // ESP32 TX -> TF-Luna RX (pin 2)

HardwareSerial lunaSerial(2);
int lastLunaDistanceCm = -1;

// -----------------------------------------------------------------
//  Safety limits
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
//  Laser state - controlled ONLY by explicit commands from the PC.
// -----------------------------------------------------------------
bool laserShouldBeOn = false;

// -----------------------------------------------------------------
//  Lock state
// -----------------------------------------------------------------
bool isCurrentlyLocked  = false;
bool wasLockedLastLoop  = false;


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
        Serial.print("ESP32 DevKit IP address: ");
        Serial.println(WiFi.localIP());

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
//  fields. This MUST match exactly what the Python side sends.
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


// =================================================================
//  MAIN PROGRAM
// =================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, LOW);

  connectToWiFi();

  initDFPlayer();
  initLuna();

  panServo.attach(PAN_PIN);
  tiltServo.attach(TILT_PIN);

  panServo.write(currentPanAngle);
  tiltServo.write(currentTiltAngle);

  tcpServer.begin();
  Serial.println("Full controller ready: motors, laser, sound, LiDAR.");
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

  static unsigned long lastLunaPrint = 0;
  if (millis() - lastLunaPrint > 1000 && lastLunaDistanceCm >= 0) {
    Serial.print("LiDAR distance: ");
    Serial.print(lastLunaDistanceCm);
    Serial.println(" cm");
    lastLunaPrint = millis();
  }

  delay(20);
}