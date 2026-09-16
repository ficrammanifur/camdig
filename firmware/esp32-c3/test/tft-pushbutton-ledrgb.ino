#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// ========================================
// TFT
// ========================================
#define TFT_SCLK 8
#define TFT_MOSI 10
#define TFT_CS   1
#define TFT_RST  3
#define TFT_DC   2

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

// ========================================
// PUSH BUTTON
// ========================================
#define BUTTON_PIN 4

// ========================================
// RGB LED
// Common Cathode
// ========================================
#define LED_R 5
#define LED_G 6
#define LED_B 7


// ========================================
// SET RGB
// ========================================
void setRGB(bool r, bool g, bool b) {
  digitalWrite(LED_R, r ? HIGH : LOW);
  digitalWrite(LED_G, g ? HIGH : LOW);
  digitalWrite(LED_B, b ? HIGH : LOW);
}


// ========================================
// TFT TEXT
// ========================================
void showText(const char* title, const char* status) {

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println(title);

  tft.drawLine(
    0, 35,
    tft.width(), 35,
    ST77XX_BLUE
  );

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(10, 55);
  tft.println(status);
}


// ========================================
// SETUP
// ========================================
void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32-C3 TFT + BUTTON + RGB");

  // ----------------------------
  // Button
  // ----------------------------
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // ----------------------------
  // RGB
  // ----------------------------
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  // RGB OFF
  setRGB(LOW, LOW, LOW);

  // ----------------------------
  // SPI
  // ----------------------------
  SPI.begin(
    TFT_SCLK,
    -1,
    TFT_MOSI,
    TFT_CS
  );

  // ----------------------------
  // TFT
  // ----------------------------
  tft.initR(INITR_BLACKTAB);

  tft.setRotation(1);

  showText(
    "ESP32-C3",
    "SYSTEM READY"
  );

  // LED biru = ready
  setRGB(LOW, LOW, HIGH);

  Serial.println("System READY");
}


// ========================================
// LOOP
// ========================================
void loop() {

  int buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == LOW) {

    Serial.println("BUTTON PRESSED");

    // LED MERAH
    setRGB(HIGH, LOW, LOW);

    showText(
      "BUTTON",
      "PRESSED!"
    );

    delay(300);

    // LED HIJAU
    setRGB(LOW, HIGH, LOW);

    showText(
      "BUTTON",
      "RELEASE"
    );

    // Tunggu tombol dilepas
    while (digitalRead(BUTTON_PIN) == LOW) {
      delay(10);
    }

    // kembali biru
    setRGB(LOW, LOW, HIGH);

    showText(
      "ESP32-C3",
      "SYSTEM READY"
    );
  }

  delay(20);
}
