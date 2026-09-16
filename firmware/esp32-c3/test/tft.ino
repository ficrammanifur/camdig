#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// =========================
// PIN ESP32-C3
// =========================
#define TFT_SCLK 8
#define TFT_MOSI 10
#define TFT_CS   1
#define TFT_RST  3  
#define TFT_DC   2

// =========================
// TFT OBJECT
// =========================
Adafruit_ST7735 tft = Adafruit_ST7735(
  TFT_CS,
  TFT_DC,
  TFT_RST
);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32-C3 + ST7735 TEST");

  // SPI custom pin
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  // ST7735 128x160
  tft.initR(INITR_BLACKTAB);

  // Rotasi
  tft.setRotation(1);

  // Background
  tft.fillScreen(ST77XX_BLACK);

  // Judul
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("ESP32-C3");

  // Garis
  tft.drawLine(
    0, 35,
    tft.width(), 35,
    ST77XX_BLUE
  );

  // Status
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(10, 50);
  tft.println("TFT ST7735");

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(10, 70);
  tft.println("SPI Connected");

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(10, 90);
  tft.println("ESP32-C3");

  // Kotak test
  tft.drawRect(
    5, 110,
    tft.width() - 10,
    40,
    ST77XX_RED
  );

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(15, 125);
  tft.println("TEST OK");
}

void loop() {
}
