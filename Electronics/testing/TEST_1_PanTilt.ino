// =================================================================
//  TEST 1 - PAN/TILT SERVOS (CALIBRATED SAFE BOUNDS)
//  Pan: 0-180 | Tilt: 45-150 (Calibrated for MG996R physical limit)
// =================================================================

#include <ESP32Servo.h>

#define SERVO_PAN_PIN  4
#define SERVO_TILT_PIN 5

// Exact safe mechanical limits for your Tilt axis
#define TILT_MIN_SAFE 45
#define TILT_MAX_SAFE 150  // Limited to 150 to prevent over-rotation/stall
#define TILT_START    90

#define PAN_MIN_SAFE  0
#define PAN_MAX_SAFE  180
#define PAN_START     90

Servo panServo;
Servo tiltServo;

// Standard pulse range
const int MIN_PULSE = 500;
const int MAX_PULSE = 2400;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== TEST 1: PAN/TILT SERVOS (SAFE 150 DEGREE TILT LIMIT) ===");
  Serial.println("Pan Pin  = GPIO4 (0 to 180)");
  Serial.println("Tilt Pin = GPIO5 (45 to 150 MAX)");
  Serial.println("");

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);

  panServo.setPeriodHertz(50);
  tiltServo.setPeriodHertz(50);

  panServo.attach(SERVO_PAN_PIN, MIN_PULSE, MAX_PULSE);
  tiltServo.attach(SERVO_TILT_PIN, MIN_PULSE, MAX_PULSE);

  // Set initial position: Pan=90, Tilt=90
  panServo.write(PAN_START);
  tiltServo.write(TILT_START);
  delay(1000);

  Serial.println(">>> READY! Try 'T150', 'T90', 'T45', or 'P180' <<<");
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toUpperCase();

    if (input.length() > 1) {
      char type = input.charAt(0);
      int requestedAngle = input.substring(1).toInt();

      if (type == 'P') {
        int targetAngle = constrain(requestedAngle, PAN_MIN_SAFE, PAN_MAX_SAFE);
        panServo.write(targetAngle);
        Serial.print("PAN set to: ");
        Serial.println(targetAngle);
      } 
      else if (type == 'T') {
        int targetAngle = constrain(requestedAngle, TILT_MIN_SAFE, TILT_MAX_SAFE);
        
        if (requestedAngle > TILT_MAX_SAFE) {
          Serial.print("LIMITED! Requested ");
          Serial.print(requestedAngle);
          Serial.print("° exceeds physical max. Constrained to ");
          Serial.println(TILT_MAX_SAFE);
        } else if (requestedAngle < TILT_MIN_SAFE) {
          Serial.print("LIMITED! Requested ");
          Serial.print(requestedAngle);
          Serial.print("° below safe min. Constrained to ");
          Serial.println(TILT_MIN_SAFE);
        } else {
          Serial.print("TILT set to: ");
          Serial.println(targetAngle);
        }

        tiltServo.write(targetAngle);
      }
    }
  }
  delay(10);
}