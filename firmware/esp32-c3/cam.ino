/*
 * ============================================================
 * ESP32-CAM - DIAGNOSTIC VERSION (FIXED)
 * Flash LED = indikator visual setiap event
 *
 * PERBAIKAN dari versi sebelumnya:
 * 1. Framing pakai MAGIC 4-byte + checksum XOR, bukan cuma
 *    0xFF 0xD8 (yang juga muncul di dalam JPEG itu sendiri,
 *    jadi rawan salah sinkron kalau ada byte hilang/rusak).
 * 2. Saat CAM terima command "PHOTO", streaming DIHENTIKAN
 *    dulu (streaming = false), supaya frame STREAM dan PHOTO
 *    tidak saling menyelip di kabel serial.
 * ============================================================
 */

#include "esp_camera.h"

// ============================================================
// CAMERA PIN AI THINKER
// ============================================================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5

#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define BUTTON_PIN 13
#define FLASH_PIN  4

#define TYPE_STREAM 0x01
#define TYPE_PHOTO  0x02

// Magic 4-byte, jauh lebih jarang "kebetulan" muncul di dalam
// data JPEG dibanding cuma 0xFF 0xD8 (yang pasti selalu ada
// di setiap frame JPEG sebagai SOI marker aslinya).
const uint8_t MAGIC[4] = {0xA5, 0x5A, 0xC3, 0x3C};

bool streaming = false;
bool photoPending = false;

// ============================================================
// KIRIM FRAME JPEG
// Format paket: [MAGIC 4][TYPE 1][LEN 4][PAYLOAD LEN byte][CHK 1]
// CHK = XOR seluruh byte payload, buat validasi sederhana di sisi
// penerima (C3) supaya frame yang rusak/kepotong bisa dideteksi
// dan dibuang, bukan malah dipaksa didekode jadi gambar rusak.
// ============================================================
bool sendJpeg(uint8_t type) {
  camera_fb_t *fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Capture gagal");
    return false;
  }

  uint32_t len = fb->len;
  uint8_t chk = 0;
  for (uint32_t i = 0; i < fb->len; i++) chk ^= fb->buf[i];

  Serial1.write(MAGIC, 4);
  Serial1.write(type);
  Serial1.write((uint8_t*)&len, 4);
  Serial1.write(fb->buf, fb->len);
  Serial1.write(chk);

  Serial1.flush();

  esp_camera_fb_return(fb);
  return true;
}

// ============================================================
// BACA PERINTAH
// ============================================================
String cmdBuf = "";

void readCommand() {
  while (Serial1.available()) {
    char c = Serial1.read();

    // ========================================================
    // INDIKATOR: flash nyala 30ms setiap byte masuk
    // Kalau flash tidak berkedip, berarti tidak ada data masuk
    // ========================================================
    digitalWrite(FLASH_PIN, HIGH);
    delay(30);
    digitalWrite(FLASH_PIN, LOW);

    if (c == '\n') {
      cmdBuf.trim();

      // Konfirmasi visual: 3 kedip cepat = perintah valid diterima
      if (cmdBuf == "STREAM") {
        Serial.println("CMD -> STREAM");
        streaming = true;
        for (int i = 0; i < 3; i++) {
          digitalWrite(FLASH_PIN, HIGH);
          delay(50);
          digitalWrite(FLASH_PIN, LOW);
          delay(50);
        }
      }
      else if (cmdBuf == "PHOTO") {
        Serial.println("CMD -> PHOTO");
        // PENTING: hentikan streaming dulu supaya frame PHOTO
        // tidak menyelip di antara frame-frame STREAM di kabel.
        streaming = false;
        photoPending = true;
        for (int i = 0; i < 2; i++) {
          digitalWrite(FLASH_PIN, HIGH);
          delay(100);
          digitalWrite(FLASH_PIN, LOW);
          delay(100);
        }
      }
      else if (cmdBuf == "STOP") {
        Serial.println("CMD -> STOP");
        streaming = false;
      }
      else {
        Serial.printf("CMD tidak dikenal: '%s'\n", cmdBuf.c_str());
      }

      cmdBuf = "";
    }
    else {
      if (cmdBuf.length() < 20) cmdBuf += c;
      else cmdBuf = "";
    }
  }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200, SERIAL_8N1, -1, 99);
  delay(500);

  Serial.println();
  Serial.println("=== ESP32-CAM DIAGNOSTIC (FIXED) ===");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(FLASH_PIN, OUTPUT);
  digitalWrite(FLASH_PIN, LOW);

  Serial1.begin(460800, SERIAL_8N1, 3, 1);

  // Camera init (sama seperti sebelumnya)
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_QVGA;
  config.jpeg_quality = 20;
  config.fb_count     = 2;
  config.grab_mode    = CAMERA_GRAB_LATEST;
  config.fb_location  = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Init GAGAL: 0x%x\n", err);
    return;
  }

  Serial.println("Camera init OK");

  // Kedip 3x tanda siap
  for (int i = 0; i < 3; i++) {
    digitalWrite(FLASH_PIN, HIGH);
    delay(150);
    digitalWrite(FLASH_PIN, LOW);
    delay(150);
  }

  Serial.println("CAM READY - diagnostic mode");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  readCommand();

  // Tombol CAM (opsional)
  static unsigned long lastBtn = 0;
  if (digitalRead(BUTTON_PIN) == LOW && millis() - lastBtn > 300) {
    lastBtn = millis();
    Serial.println("Tombol CAM ditekan");
    while (digitalRead(BUTTON_PIN) == LOW) delay(10);
  }

  // PHOTO pending
  if (photoPending) {
    Serial.println("Ambil PHOTO...");

    camera_fb_t *dummy = esp_camera_fb_get();
    if (dummy) esp_camera_fb_return(dummy);
    delay(20);

    sendJpeg(TYPE_PHOTO);
    Serial.println("PHOTO terkirim");

    photoPending = false;
    return;
  }

  // Streaming
  if (streaming) {
    sendJpeg(TYPE_STREAM);
  }
  else {
    delay(5);
  }
}
