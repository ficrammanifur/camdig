/*
 * ============================================================
 * ESP32-C3 - SPLASH + LIVE STREAM + SNAPSHOT (FIXED)
 * TFT ST7735 + TJpg_Decoder
 *
 * PERBAIKAN dari versi sebelumnya:
 * 1. Serial1 RX buffer diperbesar (default cuma ~256 byte,
 *    kekecilan untuk frame JPEG beberapa KB di 460800 baud
 *    sementara TFT sibuk menggambar via SPI -> overflow ->
 *    live stream gagal terus / layar tetap hitam).
 * 2. Framing pakai MAGIC 4-byte + checksum XOR (match dengan
 *    firmware CAM yang sudah diperbaiki), lebih tahan salah
 *    sinkron dibanding cuma cari 0xFF 0xD8.
 * 3. Ada counter debug via Serial (USB) supaya kelihatan kalau
 *    frame benar-benar masuk atau tidak.
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

// Buffer RX diperbesar supaya cukup menampung frame JPEG penuh
// walau TFT sedang sibuk menggambar (SPI ke TFT itu relatif lambat).
#define UART_RX_BUFFER_SIZE 16384

#define TYPE_STREAM 0x01
#define TYPE_PHOTO  0x02

const uint8_t MAGIC[4] = {0xA5, 0x5A, 0xC3, 0x3C};

// ============================================================
// STATE
// ============================================================
bool liveMode = true;         // true = streaming, false = freeze photo
unsigned long lastButton = 0;

// Debug counters
uint32_t framesOk = 0;
uint32_t framesFail = 0;
unsigned long lastDebugPrint = 0;

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
// SPLASH SCREEN
// ============================================================
void splashScreen() {
  // ---------- Fase 1: warna RGB bergantian dari berbagai arah ----------
  for (int x = 0; x < tft.width(); x += 4) {
    tft.fillRect(x, 0, 4, tft.height(), ST77XX_RED);
    delay(8);
  }
  delay(100);

  for (int x = tft.width() - 4; x >= 0; x -= 4) {
    tft.fillRect(x, 0, 4, tft.height(), ST77XX_GREEN);
    delay(8);
  }
  delay(100);

  for (int y = 0; y < tft.height(); y += 4) {
    tft.fillRect(0, y, tft.width(), 4, ST77XX_BLUE);
    delay(8);
  }
  delay(100);

  for (int y = tft.height() - 4; y >= 0; y -= 4) {
    tft.fillRect(0, y, tft.width(), 4, ST77XX_YELLOW);
    delay(8);
  }
  delay(150);

  // ---------- Fase 2: wipe diagonal ----------
  for (int i = 0; i < 40; i++) {
    tft.drawLine(0, i * 4, i * 4, 0, ST77XX_CYAN);
    delay(15);
  }
  for (int i = 0; i < 40; i++) {
    tft.drawLine(tft.width() - 1, i * 4, tft.width() - 1 - i * 4, 0, ST77XX_MAGENTA);
    delay(15);
  }
  delay(150);

  // ---------- Fase 3: judul ----------
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(3);
  tft.setCursor(20, 20);
  tft.println("CAMDIG");

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(3);
  tft.setCursor(20, 50);
  tft.println("Paceenihh");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(20, 90);
  tft.println("Camera Module v1.0");

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(20, 105);
  tft.println("ESP32-C3 + TFT");
  delay(1500);

  // ---------- Fase 4: loading bar ----------
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(20, 40);
  tft.println("Connecting to CAM...");

  tft.drawRect(20, 60, 120, 10, ST77XX_WHITE);

  for (int i = 0; i < 120; i += 4) {
    tft.fillRect(22 + i, 62, 4, 6, ST77XX_GREEN);
    delay(20);
  }
  delay(200);

  // ---------- Fase 5: siap ----------
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(15, 40);
  tft.println("READY");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(15, 70);
  tft.println("Starting live view...");
  delay(500);
}

// ============================================================
// TERIMA FRAME JPEG DARI CAM
// Format: [MAGIC 4][TYPE 1][LEN 4][PAYLOAD len byte][CHK 1]
// Return: buffer JPEG (harus di-free), set *outType
// ============================================================
uint8_t* receiveJpeg(uint32_t* outLen, uint8_t* outType, uint32_t timeoutMs) {
  unsigned long start = millis();
  int matched = 0; // berapa byte MAGIC yang sudah cocok berurutan

  // Cari 4-byte MAGIC secara berurutan (lebih aman daripada cuma
  // 2 byte 0xFF 0xD8, yang juga muncul di dalam data JPEG itu sendiri)
  while (millis() - start < timeoutMs) {
    if (Serial1.available()) {
      uint8_t b = Serial1.read();
      if (b == MAGIC[matched]) {
        matched++;
        if (matched == 4) break;
      } else {
        // reset, tapi cek juga apakah byte ini kebetulan cocok
        // dengan MAGIC[0] supaya tidak buang kesempatan sinkron
        matched = (b == MAGIC[0]) ? 1 : 0;
      }
    } else {
      delay(1);
    }
  }

  if (matched != 4) return NULL; // timeout, tidak ketemu MAGIC

  // Baca type
  while (Serial1.available() < 1 && millis() - start < timeoutMs) delay(1);
  if (!Serial1.available()) return NULL;
  uint8_t type = Serial1.read();

  // Baca length
  while (Serial1.available() < 4 && millis() - start < timeoutMs + 500) delay(1);
  if (Serial1.available() < 4) return NULL;

  uint32_t len = 0;
  Serial1.readBytes((uint8_t*)&len, 4);

  if (len == 0 || len > 60000) return NULL;

  uint8_t* buf = (uint8_t*)malloc(len);
  if (!buf) return NULL;

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

  if (received != len) {
    free(buf);
    return NULL;
  }

  // Baca & verifikasi checksum
  while (Serial1.available() < 1 && millis() - start < 500) delay(1);
  if (!Serial1.available()) {
    free(buf);
    return NULL;
  }
  uint8_t chkRecv = Serial1.read();
  uint8_t chkCalc = 0;
  for (uint32_t i = 0; i < len; i++) chkCalc ^= buf[i];

  if (chkRecv != chkCalc) {
    free(buf);
    return NULL; // frame rusak, dibuang, jangan dipaksa render
  }

  *outLen = len;
  *outType = type;
  return buf;
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println("=== C3 SPLASH + STREAM (FIXED) ===");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  setRGB(LOW, LOW, LOW);

  // TFT
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);

  TJpgDec.setJpgScale(2);
  TJpgDec.setSwapBytes(false);
  TJpgDec.setCallback(tft_output);

  // Splash
  Serial.println("Splash...");
  splashScreen();
  Serial.println("Splash selesai");

  // UART ke CAM - buffer RX diperbesar SEBELUM begin()
  Serial1.setRxBufferSize(UART_RX_BUFFER_SIZE);
  Serial1.begin(460800, SERIAL_8N1, CAM_RX, CAM_TX);
  delay(100);

  // Minta CAM mulai streaming
  while (Serial1.available()) Serial1.read();
  Serial1.println("STREAM");
  Serial.println("STREAM dikirim ke CAM");

  setRGB(LOW, LOW, HIGH);   // biru = live
  liveMode = true;
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  // ---------- Tombol C3 ----------
  if (digitalRead(BUTTON_PIN) == LOW && millis() - lastButton > 400) {
    lastButton = millis();

    if (liveMode) {
      // LIVE -> PHOTO
      Serial.println(">>> Mode PHOTO");
      setRGB(HIGH, LOW, LOW);

      while (Serial1.available()) Serial1.read();
      Serial1.println("PHOTO");

      uint32_t len = 0;
      uint8_t type = 0;
      unsigned long start = millis();
      uint8_t* jpeg = NULL;

      while (millis() - start < 3000) {
        jpeg = receiveJpeg(&len, &type, 1000);
        if (jpeg && type == TYPE_PHOTO) {
          Serial.printf("PHOTO diterima: %u byte\n", len);
          break;
        }
        if (jpeg) free(jpeg);
        jpeg = NULL;
      }

      if (jpeg) {
        tft.fillScreen(ST77XX_BLACK);
        JRESULT r = TJpgDec.drawJpg(0, 0, jpeg, len);
        free(jpeg);

        if (r == JDR_OK) {
          Serial.println("PHOTO tampil OK");
          setRGB(LOW, HIGH, LOW);
          liveMode = false;
        } else {
          Serial.printf("Decode gagal: %d\n", r);
          setRGB(HIGH, HIGH, LOW);
        }
      } else {
        Serial.println("PHOTO timeout");
        setRGB(HIGH, LOW, HIGH);
      }
    }
    else {
      // PHOTO -> LIVE
      Serial.println(">>> Mode LIVE");
      while (Serial1.available()) Serial1.read();
      Serial1.println("STREAM");
      setRGB(LOW, LOW, HIGH);
      liveMode = true;
    }

    while (digitalRead(BUTTON_PIN) == LOW) delay(10);
  }

  // ---------- LIVE MODE: tampilkan frame baru terus-menerus ----------
  if (liveMode) {
    uint32_t len = 0;
    uint8_t type = 0;
    uint8_t* jpeg = receiveJpeg(&len, &type, 500);

    if (jpeg) {
      if (type == TYPE_STREAM) {
        TJpgDec.drawJpg(0, 0, jpeg, len);
        framesOk++;
      }
      free(jpeg);
    } else {
      framesFail++;
    }
  }

  // Debug ringkas tiap 2 detik lewat USB Serial
  if (millis() - lastDebugPrint > 2000) {
    lastDebugPrint = millis();
    Serial.printf("Frame OK: %u | Frame gagal/timeout: %u\n", framesOk, framesFail);
  }

  delay(1);
}
