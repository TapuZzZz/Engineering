// =================================================================
//  TEST 4 - LIDAR (TF-LUNA) ONLY
//  Standalone, isolated test. Nothing else on this board is touched.
//
//  WIRING: Luna RX = GPIO17, Luna TX = GPIO18
//
//  WHAT IT DOES: continuously reads distance frames from the TF-Luna
//  and prints the distance in cm to Serial as soon as each valid
//  frame arrives. Point it at something and move your hand toward/
//  away from it - the printed numbers should change accordingly.
//  If nothing ever prints, check RX/TX wiring direction and power.
// =================================================================

#include <HardwareSerial.h>

#define LUNA_RX_PIN 17
#define LUNA_TX_PIN 18

HardwareSerial lunaSerial(2);

unsigned long lastReadingTime = 0;
bool everGotAReading = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== TEST 4: LIDAR (TF-LUNA) ONLY ===");
  Serial.println("RX = GPIO17, TX = GPIO18");
  Serial.println("");
  Serial.println("Move your hand toward/away from the sensor and watch the numbers.");
  Serial.println("");

  lunaSerial.begin(115200, SERIAL_8N1, LUNA_RX_PIN, LUNA_TX_PIN);
}

void loop() {
  if (lunaSerial.available() >= 9) {
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

    int distanceCm = frame[2] | (frame[3] << 8);
    everGotAReading = true;
    lastReadingTime = millis();

    Serial.print("Distance: ");
    Serial.print(distanceCm);
    Serial.println(" cm");
  }

  if (!everGotAReading && millis() > 3000 && millis() % 1000 < 20) {
    Serial.println("No readings yet - check RX/TX wiring and Luna power.");
  }
}
