/*
 * ============================================================
 * ESP32-C3 - UART TEST SEDERHANA
 * Tombol kirim pesan ke CAM
 * Tampilkan status di TFT, indikator di RGB
 *
 * Skema:
 *   C3 GPIO20 (RX) ◄── CAM GPIO1 (TX)
 *   C3 GPIO21 (TX) ──► CAM GPIO3 (RX)
 *   C3 GND         ──── CAM GND
 *   C3 GPIO4  ──── tombol ──── GND
 *   C3 GPIO5  → R LED
 *   C3 GPIO6  → G LED
 *   C3 GPIO7  → B LED
 *   C3 TFT:
 *     GPIO8  → SCLK
 *     GPIO10 → MOSI
 *     GPIO1  → CS
 *     GPIO3  → RST
 *     GPIO2  → DC
 * ============================================================
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// ============================================================
// TFT
// ============================================================
#define TFT_SCLK 8
#define TFT_MOSI 10
#define TFT_CS   1
#define TFT_RST  3
#define TFT_DC   2

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

// ============================================================
// TOMBOL & RGB
// ============================================================
#define BUTTON_PIN 4
#define LED_R 5
#define LED_G 6
#define LED_B 7

// ============================================================
// UART KE CAM
// ============================================================
#define CAM_RX 20
#define CAM_TX 21

// ============================================================
// SET RGB
// ============================================================
void setRGB(bool r, bool g, bool b) {
  digitalWrite(LED_R, r ? HIGH : LOW);
  digitalWrite(LED_G, g ? HIGH : LOW);
  digitalWrite(LED_B, b ? HIGH : LOW);
}

// ============================================================
// TAMPILKAN TEKS DI TFT
// ============================================================
void showScreen(const char* line1, const char* line2, const char* line3) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println(line1);

  tft.drawLine(0, 35, tft.width(), 35, ST77XX_BLUE);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(10, 45);
  tft.println(line2);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(10, 65);
  tft.println(line3);
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println("=== C3 UART TEST ===");

  // Pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  setRGB(LOW, LOW, LOW);

  // UART ke CAM
  Serial1.begin(460800, SERIAL_8N1, CAM_RX, CAM_TX);
  Serial.println("Serial1 ready (RX=20, TX=21, baud=460800)");

  // TFT
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);

  showScreen("UART TEST", "Tekan tombol", "untuk kirim");

  setRGB(LOW, LOW, HIGH);   // biru = ready
  Serial.println("System READY");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  // ---------- Tombol C3 ----------
  static unsigned long lastBtn = 0;
  if (digitalRead(BUTTON_PIN) == LOW && millis() - lastBtn > 300) {
    lastBtn = millis();

    Serial.println(">>> Tombol C3 ditekan");
    setRGB(HIGH, LOW, LOW);   // merah
    showScreen("KIRIM", "Mengirim...", "");

    // Kirim pesan ke CAM
    Serial1.println("HALO_DARI_C3");
    Serial.println("Pesan dikirim: HALO_DARI_C3");

    // Tunggu balasan maksimal 2 detik
    unsigned long start = millis();
    bool dapatBalasan = false;
    String balasan = "";

    while (millis() - start < 2000) {
      if (Serial1.available()) {
        balasan = Serial1.readStringUntil('\n');
        balasan.trim();
        dapatBalasan = true;
        break;
      }
      delay(1);
    }

    if (dapatBalasan) {
      Serial.printf("Balasan dari CAM: '%s'\n", balasan.c_str());
      setRGB(LOW, HIGH, LOW);   // hijau
      showScreen("BALASAN", balasan.c_str(), "OK!");
    } else {
      Serial.println("Timeout, tidak ada balasan");
      setRGB(HIGH, LOW, HIGH);   // magenta
      showScreen("TIMEOUT", "Tidak ada", "balasan");
    }

    while (digitalRead(BUTTON_PIN) == LOW) delay(10);
    setRGB(LOW, LOW, HIGH);
    showScreen("UART TEST", "Tekan tombol", "untuk kirim");
  }

  // ---------- Pesan masuk dari CAM ----------
  if (Serial1.available()) {
    String msg = Serial1.readStringUntil('\n');
    msg.trim();

    Serial.printf("Pesan masuk dari CAM: '%s'\n", msg.c_str());
    setRGB(HIGH, HIGH, LOW);   // kuning
    showScreen("DARI CAM", msg.c_str(), "");
    delay(1500);
    setRGB(LOW, LOW, HIGH);
    showScreen("UART TEST", "Tekan tombol", "untuk kirim");
  }

  delay(10);
}
