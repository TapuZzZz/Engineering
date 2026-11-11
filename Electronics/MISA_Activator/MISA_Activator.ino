#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>

// ---------- Settings ----------
struct WifiNetwork { const char* ssid; const char* pass; };

const WifiNetwork NETWORKS[] = {
  {"Amal-Student", "Amal1@st"},
};

const char* HOST_NAME = "ESP-ACTIVATOR";
const int UDP_PORT = 5006;

const int MIN_ANGLE = 0;
const int MAX_ANGLE = 180;
const int CENTER_ANGLE = 90;

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

// ---------- Globals ----------
WiFiMulti wifiMulti;
WiFiUDP udp;
Servo panServo;
Servo tiltServo;

// ---------- Laser ----------
void initLaser() {
  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, LOW);  // laser off by default
}

// ---------- Network ----------
void connectWifi() {
  for (const WifiNetwork& n : NETWORKS) wifiMulti.addAP(n.ssid, n.pass);
  while (wifiMulti.run() != WL_CONNECTED) delay(500);
}

void startMdns() {
  if (MDNS.begin(HOST_NAME)) Serial.println("Name: ESP-ACTIVATOR.local");
}

void startUdp() {
  udp.begin(UDP_PORT);
}

bool readMessage(char* buffer, size_t size) {
  if (udp.parsePacket() <= 0) return false;
  int length = udp.read(buffer, size - 1);
  buffer[length] = '\0';
  return true;
}

// ---------- Servos ----------
int clampAngle(int angle) {
  return constrain(angle, MIN_ANGLE, MAX_ANGLE);
}

void moveServos(int pan, int tilt) {
  panServo.write(clampAngle(pan));
  tiltServo.write(clampAngle(tilt));
}

void initServos() {
  panServo.setPeriodHertz(50);
  tiltServo.setPeriodHertz(50);
  panServo.attach(PAN_PIN, 500, 2400);
  tiltServo.attach(TILT_PIN, 500, 2400);
  moveServos(CENTER_ANGLE, CENTER_ANGLE);
}

bool parseAngles(const char* message, int& pan, int& tilt) {
  return sscanf(message, "P,%d,%d", &pan, &tilt) == 2;
}

void handleMessage(const char* message) {
  int pan, tilt;
  if (!parseAngles(message, pan, tilt)) return;
  Serial.printf("pan=%d tilt=%d\n", pan, tilt);
  moveServos(pan, tilt);
}

// ---------- Main ----------
void setup() {
  initLaser();
  Serial.begin(115200);
  initServos();
  connectWifi();
  startMdns();
  startUdp();
  Serial.println(WiFi.localIP());
}

void loop() {
  char message[32];
  if (readMessage(message, sizeof(message))) handleMessage(message);
}/