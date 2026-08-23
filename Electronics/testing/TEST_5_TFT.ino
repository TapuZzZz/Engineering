// =================================================================
//  TEST 5 - TFT SCREEN (ST7735) ONLY
//  Standalone, isolated test. Nothing else on this board is touched.
//
//  WIRING: CS = GPIO11, RST = GPIO12, DC = GPIO9, MOSI = GPIO10,
//          SCK = GPIO15
//
//  WHAT IT DOES: initializes the screen, fills it with a sequence
//  of solid colors (so you can check the whole panel lights up
//  correctly with no dead pixels/lines), then draws a border and
//  text. Watch for the offset artifact seen on the previous board
//  (a noisy/misaligned edge) - if present, the col/row offset
//  constants below need re-calibration for THIS physical screen.
// =================================================================

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#define TFT_CS   11
#define TFT_RST  12
#define TFT_DC   9
#define TFT_MOSI 10
#define TFT_SCK  15

class TftWithOffset : public Adafruit_ST7735 {
public:
  TftWithOffset(SPIClass* spiBus, int8_t csPin, int8_t dcPin, int8_t rstPin)
    : Adafruit_ST7735(spiBus, csPin, dcPin, rstPin) {}

  using Adafruit_ST7735::setColRowStart;
};

TftWithOffset tft = TftWithOffset(&SPI, TFT_CS, TFT_DC, TFT_RST);

// Carried over from the previous board's calibration - RE-VERIFY
// visually on this screen. If the border test below looks shifted
// or clipped on one edge, adjust these two numbers up/down by 1-2
// and re-flash until the border sits flush on all four sides.
const int8_t TFT_COL_OFFSET = 2;
const int8_t TFT_ROW_OFFSET = 1;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== TEST 5: TFT SCREEN ONLY ===");
  Serial.println("CS=11 RST=12 DC=9 MOSI=10 SCK=15");
  Serial.println("");

  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, HIGH);
  delay(100);
  digitalWrite(TFT_RST, LOW);
  delay(100);
  digitalWrite(TFT_RST, HIGH);
  delay(100);

  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);
  SPI.setFrequency(1000000);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.setColRowStart(TFT_COL_OFFSET, TFT_ROW_OFFSET);

  Serial.println("Screen initialized. Running color fill test...");

  tft.fillScreen(ST7735_RED);
  Serial.println("RED - check full panel is red, no dead areas.");
  delay(1000);

  tft.fillScreen(ST7735_GREEN);
  Serial.println("GREEN");
  delay(1000);

  tft.fillScreen(ST7735_BLUE);
  Serial.println("BLUE");
  delay(1000);

  tft.fillScreen(ST7735_WHITE);
  Serial.println("WHITE - look closely for any noisy/misaligned edge.");
  delay(1000);

  tft.fillScreen(ST7735_BLACK);

  // Border test - should sit flush against all 4 edges with no gap
  // or clipping. If it doesn't, adjust TFT_COL_OFFSET/TFT_ROW_OFFSET.
  tft.drawRect(0, 0, tft.width(), tft.height(), ST7735_YELLOW);

  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setCursor(10, 10);
  tft.print("TFT TEST OK");
  tft.setCursor(10, 25);
  tft.print("Border should be flush");
  tft.setCursor(10, 40);
  tft.print("on all 4 edges.");

  Serial.println("Border + text drawn. Check border alignment on all edges.");
  Serial.println("");
  Serial.println("=== TEST COMPLETE ===");
}

void loop() {
  // Nothing to do - static test pattern stays on screen.
}
