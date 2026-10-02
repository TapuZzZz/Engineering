// =================================================================
//  TEST 3 - LASER ONLY
//  Standalone, isolated test. Nothing else on this board is touched.
//
//  WIRING: GPIO6 -> 220ohm resistor -> Base of 2N2222A transistor
//          (laser module wired as described earlier: transistor as
//          a low-side switch)
//
//  SAFETY REMINDER: even a small laser module can cause eye damage
//  at close range. Do not point it at yourself, anyone else, or any
//  reflective surface while this test is running. This sketch turns
//  the laser on/off automatically in a short blink pattern purely so
//  you get feedback without needing to type Serial commands - point
//  it somewhere safe (a wall, the floor) before powering it on.
//
//  WHAT IT DOES: blinks the laser ON for 200ms, OFF for 800ms,
//  repeatedly, printing each transition to Serial. If the laser
//  never lights up despite the "ON" prints, the problem is in the
//  transistor wiring or the laser module's own power, not the GPIO.
// =================================================================

const int LASER_PIN = 6;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, LOW);

  Serial.println("=== TEST 3: LASER ONLY ===");
  Serial.println("GPIO6 -> 220ohm -> 2N2222A base");
  Serial.println("");
  Serial.println("SAFETY: point the laser at a safe surface (wall/floor),");
  Serial.println("never at eyes, before it starts blinking.");
  Serial.println("");
  delay(2000);   // Time to physically aim it safely before it starts.

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
