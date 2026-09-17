/*
 * ============================================================
 * ESP32-C3 - UART + TFT + KAMERA
 * Tombol -> kirim PHOTO -> terima JPEG -> tampilkan di TFT
 * ============================================================
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <TJpg_Decoder.h>

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
// CALLBACK TJpg_Decoder
// ============================================================
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.drawRGBBitmap(x, y, bitmap, w, h);
  return 1;
}

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
// TERIMA FRAME JPEG DARI CAM
// Return buffer JPEG (harus di-free), atau NULL kalau gagal
// ============================================================
uint8_t* receiveJpeg(uint32_t* outLen, uint32_t timeoutMs) {
  // Cari header 0xFF 0xD8
  unsigned long start = millis();
  bool found = false;

  while (millis() - start < timeoutMs) {
    if (Serial1.available() >= 2) {
      uint8_t b1 = Serial1.read();
      uint8_t b2 = Serial1.peek();
      if (b1 == 0xFF && b2 == 0xD8) {
        Serial1.read();   // buang 0xD8
        found = true;
        break;
      }
    }
    delay(1);
  }

  if (!found) {
    Serial.println("Timeout header");
    return NULL;
  }

  // Baca length
  while (Serial1.available() < 4 && millis() - start < timeoutMs + 1000) delay(1);
  if (Serial1.available() < 4) {
    Serial.println("Timeout length");
    return NULL;
  }

  uint32_t len = 0;
  Serial1.readBytes((uint8_t*)&len, 4);
  Serial.printf("Length: %u byte\n", len);

  if (len == 0 || len > 60000) {
    Serial.println("Length tidak wajar");
    return NULL;
  }

  // Alokasi
  uint8_t* buf = (uint8_t*)malloc(len);
  if (!buf) {
    Serial.println("malloc gagal");
    return NULL;
  }

  // Baca data
  uint32_t received = 0;
  start = millis();
  while (received < len && millis() - start < timeoutMs) {
    if (Serial1.available()) {
      uint32_t n = min((uint32_t)Serial1.available(), len - received);
      Serial1.readBytes(buf + received, n);
      received += n;
      start = millis();
    }
    delay(1);
  }

  Serial.printf("Diterima: %u / %u\n", received, len);

  if (received != len) {
    free(buf);
    return NULL;
  }

  // Baca footer
  while (Serial1.available() < 2 && millis() - start < 1000) delay(1);
  uint8_t e1 = Serial1.read();
  uint8_t e2 = Serial1.read();
  Serial.printf("Footer: %02X %02X\n", e1, e2);

  if (e1 != 0xFF || e2 != 0xD9) {
    free(buf);
    return NULL;
  }

  *outLen = len;
  return buf;
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("=== C3 UART + KAMERA ===");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  setRGB(LOW, LOW, LOW);

  // UART
  Serial1.begin(460800, SERIAL_8N1, CAM_RX, CAM_TX);
  Serial.println("Serial1 ready (RX=20, TX=21, baud=460800)");

  // TFT
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);

  // TJpgDec
  TJpgDec.setJpgScale(2);
  TJpgDec.setSwapBytes(false);   // <-- false
  TJpgDec.setCallback(tft_output);
  
  showScreen("KAMERA", "Tekan tombol", "untuk foto");

  setRGB(LOW, LOW, HIGH);
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

    Serial.println(">>> Tombol ditekan, minta PHOTO");

    // Bersihkan buffer
    while (Serial1.available()) Serial1.read();

    setRGB(HIGH, LOW, LOW);
    showScreen("PHOTO", "Menunggu...", "");

    // Kirim perintah
    Serial1.println("PHOTO");

    // Terima JPEG
    uint32_t len = 0;
    uint8_t* jpeg = receiveJpeg(&len, 5000);

    if (jpeg) {
      Serial.println("Decode...");
      tft.fillScreen(ST77XX_BLACK);
      JRESULT r = TJpgDec.drawJpg(0, 0, jpeg, len);
      Serial.printf("Decode result: %d\n", r);
      free(jpeg);

      if (r == JDR_OK) {
        setRGB(LOW, HIGH, LOW);
        Serial.println("TAMPIL OK");
        delay(2000);
      } else {
        setRGB(HIGH, HIGH, LOW);
        showScreen("ERROR", "Decode", "gagal");
        delay(2000);
      }
    } else {
      setRGB(HIGH, LOW, HIGH);
      showScreen("ERROR", "Tidak ada", "foto");
      delay(2000);
    }

    while (digitalRead(BUTTON_PIN) == LOW) delay(10);
    setRGB(LOW, LOW, HIGH);
    showScreen("KAMERA", "Tekan tombol", "untuk foto");
  }

  // ---------- Pesan teks dari CAM ----------
  if (Serial1.available() >= 2) {
    uint8_t b1 = Serial1.peek();
    if (b1 != 0xFF) {
      // Bukan header JPEG, baca sebagai teks
      String msg = Serial1.readStringUntil('\n');
      msg.trim();
      if (msg.length() > 0) {
        Serial.printf("Pesan CAM: '%s'\n", msg.c_str());
        setRGB(HIGH, HIGH, LOW);
        showScreen("DARI CAM", msg.c_str(), "");
        delay(1500);
        setRGB(LOW, LOW, HIGH);
        showScreen("KAMERA", "Tekan tombol", "untuk foto");
      }
    }
  }

  delay(10);
}
