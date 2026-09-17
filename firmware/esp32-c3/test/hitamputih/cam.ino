/*
 * ============================================================
 * ESP32-CAM - UART KAMERA + TOMBOL
 *
 * Perintah dari C3:
 *   PHOTO\n  -> capture dan kirim 1 frame JPEG
 *
 * Frame JPEG ke C3:
 *   FF D8 [4-byte length] [JPEG data] FF D9
 *
 * Tombol CAM (GPIO13) -> kirim "HALO_DARI_CAM"
 *
 * UART: RX=GPIO3, TX=GPIO1, BAUD=460800
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

// ============================================================
// PIN
// ============================================================
#define BUTTON_PIN 13
#define FLASH_PIN  4

// ============================================================
// KIRIM SATU FRAME JPEG
// ============================================================
bool sendJpeg() {
  camera_fb_t *fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Capture gagal");
    Serial1.println("CAM_ERROR");
    return false;
  }

  uint32_t len = fb->len;

  Serial.printf("Kirim JPEG: %u byte\n", len);

  // Header
  Serial1.write(0xFF);
  Serial1.write(0xD8);

  // Length (little-endian 4 byte)
  Serial1.write((uint8_t*)&len, 4);

  // Data JPEG
  Serial1.write(fb->buf, fb->len);

  // Footer
  Serial1.write(0xFF);
  Serial1.write(0xD9);

  Serial1.flush();

  esp_camera_fb_return(fb);
  return true;
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  // Remap Serial supaya tidak konflik dengan Serial1 (GPIO1/GPIO3)
  Serial.begin(115200, SERIAL_8N1, -1, 99);
  delay(500);

  Serial.println();
  Serial.println("=== ESP32-CAM UART + CAM ===");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(FLASH_PIN, OUTPUT);
  digitalWrite(FLASH_PIN, LOW);

  // UART ke C3
  Serial1.begin(460800, SERIAL_8N1, 3, 1);
  Serial.println("Serial1 ready (RX=3, TX=1, baud=460800)");

  // ==========================================================
  // CAMERA
  // ==========================================================
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
  config.frame_size   = FRAMESIZE_QVGA;   // 320x240
  config.jpeg_quality = 18;
  config.fb_count     = 2;
  config.grab_mode    = CAMERA_GRAB_LATEST;
  config.fb_location  = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;

  Serial.println("Init kamera...");
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Init GAGAL: 0x%x\n", err);
    Serial1.println("CAM_ERROR");
    return;
  }

  Serial.println("Camera init OK");

  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    Serial.printf("Sensor PID: 0x%02X\n", s->id.PID);
    if (s->id.PID == OV3660_PID) {
      s->set_vflip(s, 1);
      s->set_brightness(s, 1);
      s->set_saturation(s, -2);
    }
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

  // Kedip 3x tanda siap
  for (int i = 0; i < 3; i++) {
    digitalWrite(FLASH_PIN, HIGH);
    delay(150);
    digitalWrite(FLASH_PIN, LOW);
    delay(150);
  }

  Serial.println("CAM READY");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  // ---------- Tombol CAM ----------
  static unsigned long lastBtn = 0;
  if (digitalRead(BUTTON_PIN) == LOW && millis() - lastBtn > 300) {
    lastBtn = millis();

    Serial.println("Tombol CAM ditekan");
    Serial1.println("HALO_DARI_CAM");

    digitalWrite(FLASH_PIN, HIGH);
    delay(100);
    digitalWrite(FLASH_PIN, LOW);

    while (digitalRead(BUTTON_PIN) == LOW) delay(10);
  }

  // ---------- Perintah dari C3 ----------
  if (Serial1.available()) {
    String cmd = Serial1.readStringUntil('\n');
    cmd.trim();

    Serial.printf("Perintah dari C3: '%s'\n", cmd.c_str());

    if (cmd == "PHOTO") {
      Serial.println("Mengambil foto...");

      // Buang frame lama
      camera_fb_t *dummy = esp_camera_fb_get();
      if (dummy) esp_camera_fb_return(dummy);
      delay(50);

      sendJpeg();
    }
  }

  delay(5);
}
