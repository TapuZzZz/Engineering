/*
  ESP32 #2 - בקר לייזר / LED / סאונד
  ---------------------------------------------------------
  שרת TCP שמאזין לפקודות "ON" / "OFF" מהמחשב.
  בטיחות: אם לא מתקבלת פקודת "ON" חדשה תוך HEARTBEAT_TIMEOUT_MS,
  המערכת מכבה את עצמה אוטומטית (watchdog).
  כלומר: הלייזר דולק רק כל עוד המחשב שולח "ON" שוב ושוב
  (למשל כל עוד המשתמש מחזיק מקש) - לא ברצף אינסופי עצמאי.
*/

#include "DFRobotDFPlayerMini.h"
#include <WiFi.h>

// ---------- הגדרות WiFi ----------
const char* ssid     = "TP-Link- Salon";
const char* password = "03032007";
const uint16_t TCP_PORT = 4444;

WiFiServer server(TCP_PORT);
WiFiClient client;

// ---------- פינים ----------
const int laserPin     = 26;
const int ledStatusPin = 25;
const int rxPin = 4;
const int txPin = 5;

HardwareSerial mySerial(2);
DFRobotDFPlayerMini myDFPlayer;

// ---------- מצב ----------
bool firing = false;
unsigned long lastHeartbeat = 0;
// 400ms היה קצר מדי - עומס/ג'יטר רגיל ברשת ביתית (במיוחד עם מצלמה
// ששולחת פריימים במקביל) גורם לפקודות "ON" להגיע באיחור, וזה הפעיל
// את ה-watchdog שוב ושוב -> הבהוב. 1200ms נותן מרווח נשימה סביר
// ועדיין מבטיח כיבוי מהיר אם החיבור באמת נופל.
const unsigned long HEARTBEAT_TIMEOUT_MS = 1200;
const unsigned long MAX_FIRE_DURATION_MS = 8000; // הגבלת זמן ירי רציף מקסימלי (בטיחות נוספת) - במצב טוגל אפשר שיישאר דלוק זמן ארוך יותר מ"החזקה", אז הגדלתי; אם תרצה שזה יכבה מוקדם יותר, תלחץ שוב על הרווח
unsigned long fireStartTime = 0;

void setLaser(bool on) {
  if (on && !firing) {
    fireStartTime = millis();
    digitalWrite(laserPin, HIGH);
    digitalWrite(ledStatusPin, HIGH);
    myDFPlayer.play(1);
    firing = true;
    Serial.println("[FIRE] ON");
  } else if (!on && firing) {
    digitalWrite(laserPin, LOW);
    digitalWrite(ledStatusPin, LOW);
    myDFPlayer.stop();
    firing = false;
    Serial.println("[FIRE] OFF");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(laserPin, OUTPUT);
  pinMode(ledStatusPin, OUTPUT);
  digitalWrite(laserPin, LOW);
  digitalWrite(ledStatusPin, LOW);

  mySerial.begin(9600, SERIAL_8N1, rxPin, txPin);
  if (myDFPlayer.begin(mySerial)) {
    myDFPlayer.volume(25);
    Serial.println("DFPlayer OK");
  } else {
    Serial.println("DFPlayer FAILED - check wiring");
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.println("TCP server started on port " + String(TCP_PORT));
}

void loop() {
  // קבלת לקוח חדש אם אין כרגע
  if (!client || !client.connected()) {
    client = server.available();
    if (client) {
      client.setNoDelay(true);  // מבטל את Nagle's algorithm - שולח כל פקודה מיד
      Serial.println("[CTRL] Client connected");
    }
  }

  // קריאת פקודות
  if (client && client.connected() && client.available()) {
    String cmd = client.readStringUntil('\n');
    cmd.trim();
    if (cmd == "ON") {
      lastHeartbeat = millis();
      setLaser(true);
    } else if (cmd == "OFF") {
      setLaser(false);
    }
  }

  // ---------- Watchdog ----------
  // אם דולק ולא הגיע heartbeat בזמן - כיבוי כפוי
  if (firing && (millis() - lastHeartbeat > HEARTBEAT_TIMEOUT_MS)) {
    Serial.println("[WATCHDOG] Heartbeat timeout -> forcing OFF");
    setLaser(false);
  }

  // הגבלת משך ירי מקסימלי גם אם ה-heartbeat ממשיך להגיע
  if (firing && (millis() - fireStartTime > MAX_FIRE_DURATION_MS)) {
    Serial.println("[SAFETY] Max fire duration reached -> forcing OFF");
    setLaser(false);
  }

  // אם הלקוח התנתק - כיבוי מיידי
  if (firing && (!client || !client.connected())) {
    Serial.println("[SAFETY] Client disconnected -> forcing OFF");
    setLaser(false);
  }
}
