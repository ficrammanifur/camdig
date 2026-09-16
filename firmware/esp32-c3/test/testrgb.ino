#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#define TFT_SCLK 8
#define TFT_MOSI 10
#define TFT_CS   1
#define TFT_RST  3
#define TFT_DC   2

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

#define LED_R 5
#define LED_G 6
#define LED_B 7

void setRGB(bool r, bool g, bool b) {
  digitalWrite(LED_R, r ? HIGH : LOW);
  digitalWrite(LED_G, g ? HIGH : LOW);
  digitalWrite(LED_B, b ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("C3 + TFT test");

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  setRGB(LOW, LOW, LOW);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);

  tft.fillScreen(ST77XX_RED);
  delay(500);
  tft.fillScreen(ST77XX_GREEN);
  delay(500);
  tft.fillScreen(ST77XX_BLUE);
  delay(500);
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 30);
  tft.println("TFT OK");
  tft.setCursor(10, 60);
  tft.println("C3 OK");

  setRGB(LOW, LOW, HIGH);   // biru = ready
  Serial.println("Semua OK");
}

void loop() {
  static unsigned long last = 0;
  if (millis() - last > 2000) {
    last = millis();
    Serial.println("Loop...");
  }
  delay(20);
}
