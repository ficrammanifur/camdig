/*
 * ============================================================
 * ESP32-CAM - ESP-NOW TEST (TANPA KAMERA)
 * Terima pesan dari C3, balas
 * Kirim pesan saat tombol CAM ditekan
 * ============================================================
 */

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define BUTTON_PIN 13
#define FLASH_PIN  4

// ============================================================
// GANTI DENGAN MAC ADDRESS ESP32-C3 ANDA
// Format: {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}
// ============================================================
uint8_t c3Mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // <-- GANTI!

// ============================================================
// CALLBACK: pesan diterima
// ============================================================
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  Serial.printf("Pesan masuk dari %02X:%02X:%02X:%02X:%02X:%02X\n",
    info->src_addr[0], info->src_addr[1], info->src_addr[2],
    info->src_addr[3], info->src_addr[4], info->src_addr[5]);

  Serial.printf("Isi: ");
  for (int i = 0; i < len; i++) Serial.print((char)data[i]);
  Serial.println();

  // Kedip flash sebagai indikator
  digitalWrite(FLASH_PIN, HIGH);
  delay(100);
  digitalWrite(FLASH_PIN, LOW);

  // Balas
  const char* balas = "CAM_TERIMA";
  esp_now_send(c3Mac, (uint8_t*)balas, strlen(balas));
  Serial.println("Balasan terkirim: CAM_TERIMA");
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== ESP32-CAM ESP-NOW TEST ===");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(FLASH_PIN, OUTPUT);
  digitalWrite(FLASH_PIN, LOW);

  // WiFi mode station, tapi tidak connect ke AP
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Tampilkan MAC CAM
  Serial.print("MAC CAM: ");
  Serial.println(WiFi.macAddress());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init GAGAL");
    while (true) delay(1000);
  }
  Serial.println("ESP-NOW init OK");

  // Daftarkan callback
  esp_now_register_recv_cb(onDataRecv);

  // Tambahkan peer C3
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, c3Mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Gagal tambah peer C3");
  } else {
    Serial.println("Peer C3 ditambahkan");
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
  // Tombol CAM
  static unsigned long lastBtn = 0;
  if (digitalRead(BUTTON_PIN) == LOW && millis() - lastBtn > 300) {
    lastBtn = millis();

    Serial.println("Tombol CAM ditekan");
    const char* pesan = "HALO_DARI_CAM";
    esp_err_t result = esp_now_send(c3Mac, (uint8_t*)pesan, strlen(pesan));

    if (result == ESP_OK) {
      Serial.println("Pesan terkirim ke C3");
      digitalWrite(FLASH_PIN, HIGH);
      delay(100);
      digitalWrite(FLASH_PIN, LOW);
    } else {
      Serial.println("Gagal kirim");
    }

    while (digitalRead(BUTTON_PIN) == LOW) delay(10);
  }

  delay(10);
}
