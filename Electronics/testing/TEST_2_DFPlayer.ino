// =================================================================
//  TEST 2 - DFPLAYER + SPEAKER ONLY
//  Standalone, isolated test. Nothing else on this board is touched.
//
//  WIRING: DFPlayer RX = GPIO13, DFPlayer TX = GPIO14
//
//  WHAT IT DOES: initializes the DFPlayer and plays track 1 (needs
//  a microSD card with 0001.mp3 in the root or /mp3 folder). Prints
//  clear OK/FAILED status to Serial. Type any track number + Enter
//  in Serial Monitor (115200 baud) to replay/test a different track.
// =================================================================

#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>

#define DFPLAYER_RX_PIN 13
#define DFPLAYER_TX_PIN 14

HardwareSerial dfPlayerSerial(1);
DFRobotDFPlayerMini dfPlayer;
bool dfPlayerIsReady = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== TEST 2: DFPLAYER + SPEAKER ONLY ===");
  Serial.println("RX = GPIO13, TX = GPIO14");
  Serial.println("");

  dfPlayerSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);

  if (!dfPlayer.begin(dfPlayerSerial)) {
    Serial.println("RESULT: FAILED - DFPlayer not detected.");
    Serial.println("-> Check RX/TX wiring direction, and DFPlayer 5V/GND power.");
    return;
  }

  dfPlayerIsReady = true;
  dfPlayer.volume(20);
  Serial.println("RESULT: OK - DFPlayer detected. Playing track 1...");
  dfPlayer.play(1);

  Serial.println("");
  Serial.println("If you heard nothing despite 'OK' above: check speaker soldering,");
  Serial.println("or confirm the SD card has 0001.mp3 in root or /mp3 folder.");
  Serial.println("");
  Serial.println("Type a track number + Enter to test another track.");
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() > 0 && dfPlayerIsReady) {
      int trackNumber = input.toInt();
      if (trackNumber > 0) {
        Serial.print("Playing track ");
        Serial.println(trackNumber);
        dfPlayer.play(trackNumber);
      }
    }
  }
  delay(20);
}
