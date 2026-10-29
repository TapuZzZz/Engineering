// =================================================================
//  TEST 3 - LASER ONLY
//
//  WIRING: GPIO6 -> 220ohm resistor -> Base of 2N2222A transistor
//          (laser module wired as described earlier: transistor as
//          a low-side switch)
// =================================================================

const int LASER_PIN = 6;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, LOW);

  Serial.println("=== TEST 3: LASER ONLY ===");
  Serial.println("GPIO6 -> 1k -> 2N2222A base");
  Serial.println("");
  delay(2000);   
  Serial.println("Starting blink test...");
}

void loop() {
  digitalWrite(LASER_PIN, HIGH);
  Serial.println("Laser: ON");
  delay(2000);

  digitalWrite(LASER_PIN, LOW);
  Serial.println("Laser: OFF");
  delay(3000);
}
