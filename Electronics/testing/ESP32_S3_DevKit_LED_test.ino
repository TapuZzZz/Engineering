// =================================================================
//  ESP32-S3 DEVKIT  -  ONBOARD RGB LED TEST
//  Basic standalone sketch. Plays with the built-in addressable
//  RGB LED (WS2812) that's soldered onto the DevKitC-1 board itself
//  - not a component you wired, it's already on the board.
//
//  PIN: most ESP32-S3-DevKitC-1 boards use GPIO38 for this LED,
//  some revisions use GPIO48 instead. If nothing lights up, try
//  changing LED_PIN below to 48 and reflash.
//
//  Needs the "Adafruit NeoPixel" library - install via
//  Arduino IDE: Tools > Manage Libraries > search "Adafruit NeoPixel"
// =================================================================

#include <Adafruit_NeoPixel.h>

#define LED_PIN    38   // Try 48 if this doesn't work on your board revision
#define LED_COUNT  1

Adafruit_NeoPixel rgbLed(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== ONBOARD RGB LED TEST ===");
  Serial.print("Using GPIO");
  Serial.println(LED_PIN);

  rgbLed.begin();
  rgbLed.setBrightness(50);   // 0-255, keep it low - this LED is bright up close
  rgbLed.show();              // Initialize to off
}

void loop() {
  rgbLed.setPixelColor(0, rgbLed.Color(255, 0, 0));  // Red
  rgbLed.show();
  Serial.println("RED");
  delay(500);

  rgbLed.setPixelColor(0, rgbLed.Color(0, 255, 0));  // Green
  rgbLed.show();
  Serial.println("GREEN");
  delay(500);

  rgbLed.setPixelColor(0, rgbLed.Color(0, 0, 255));  // Blue
  rgbLed.show();
  Serial.println("BLUE");
  delay(500);

  rgbLed.setPixelColor(0, rgbLed.Color(0, 0, 0));    // Off
  rgbLed.show();
  Serial.println("OFF");
  delay(500);
}
