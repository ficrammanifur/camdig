# 📷 CAMDIG Paceenihh

> **Camera Digital Module — ESP32-CAM + ESP32-C3 + TFT ST7735**

CAMDIG Paceenihh adalah modul kamera digital berbasis **ESP32-CAM AI Thinker** yang terhubung ke **ESP32-C3** sebagai controller/display.

Sistem menggunakan komunikasi **UART 460800 baud** untuk mengirim gambar JPEG dari ESP32-CAM ke ESP32-C3. ESP32-C3 menerima frame, melakukan validasi paket, kemudian menampilkan gambar pada **TFT ST7735** menggunakan `TJpg_Decoder`.

---

## ✨ Fitur

* 📷 Capture gambar menggunakan ESP32-CAM
* 🎥 Live camera streaming
* 🖼️ Freeze/snapshot photo
* 🔄 Tombol untuk berpindah:

  * `LIVE → PHOTO`
  * `PHOTO → LIVE`
* 📡 Komunikasi UART 460800 baud
* 🔐 JPEG packet framing dengan:

  * 4-byte MAGIC
  * packet type
  * payload length
  * XOR checksum
* 🌈 RGB LED sebagai indikator status
* 💡 Flash LED ESP32-CAM sebagai indikator komunikasi
* 🖥️ Splash screen pada TFT saat startup
* 🔋 Battery powered menggunakan Li-Po 3.7 V 1000 mAh

---

# 🧩 System Architecture

```text
                    ┌─────────────────────┐
                    │    Li-Po 3.7 V      │
                    │      1000 mAh       │
                    └──────────┬──────────┘
                               │
                        ┌──────▼──────┐
                        │   TP4056    │
                        │ USB Type-C  │
                        └──────┬──────┘
                               │
                     Power Distribution
                               │
             ┌─────────────────┴─────────────────┐
             │                                   │
      ┌──────▼──────┐                     ┌──────▼──────┐
      │  ESP32-CAM  │                     │  ESP32-C3   │
      │ AI Thinker  │                     │ Controller  │
      └──────┬──────┘                     └──────┬──────┘
             │                                   │
             │ UART 460800                       │ SPI
             │                                   │
             └──────────────►────────────────────┘
                         JPEG DATA
                               │
                        ┌──────▼──────┐
                        │ TFT ST7735  │
                        │   Display   │
                        └─────────────┘
```

---

# 📷 ESP32-CAM

## Camera Module

Board yang digunakan:

**AI Thinker ESP32-CAM**

### Camera Pin Configuration

| Fungsi | GPIO |
| ------ | ---: |
| PWDN   |   32 |
| RESET  |   -1 |
| XCLK   |    0 |
| SIOD   |   26 |
| SIOC   |   27 |
| Y9     |   35 |
| Y8     |   34 |
| Y7     |   39 |
| Y6     |   36 |
| Y5     |   21 |
| Y4     |   19 |
| Y3     |   18 |
| Y2     |    5 |
| VSYNC  |   25 |
| HREF   |   23 |
| PCLK   |   22 |

### Camera Configuration

```cpp
pixel_format = PIXFORMAT_JPEG
frame_size   = FRAMESIZE_QVGA
jpeg_quality = 20
fb_count     = 2
grab_mode    = CAMERA_GRAB_LATEST
```

Resolusi yang digunakan:

```text
QVGA
320 × 240 pixel
```

---

# 🔌 ESP32-CAM UART

UART digunakan untuk komunikasi antara ESP32-CAM dan ESP32-C3.

| ESP32-CAM | Fungsi | ESP32-C3   |
| --------- | ------ | ---------- |
| GPIO 1    | TX     | GPIO 20 RX |
| GPIO 3    | RX     | GPIO 21 TX |
| GND       | Ground | GND        |

### UART Configuration

```cpp
Baudrate : 460800
Format   : 8N1
```

```text
ESP32-CAM                 ESP32-C3

GPIO 1 TX ──────────────► GPIO 20 RX
GPIO 3 RX ◄────────────── GPIO 21 TX

GND      ──────────────── GND
```

> ⚠️ TX harus menuju RX dan RX harus menuju TX.

---

# 📺 ESP32-C3 + TFT ST7735

ESP32-C3 berfungsi sebagai:

* Controller
* UART receiver
* JPEG decoder
* Display controller
* User interface

## TFT Pinout

| TFT  | ESP32-C3 |
| ---- | -------: |
| SCLK |   GPIO 8 |
| MOSI |  GPIO 10 |
| CS   |   GPIO 1 |
| RST  |   GPIO 3 |
| DC   |   GPIO 2 |

Konfigurasi:

```cpp
SPI.begin(
    TFT_SCLK,
    -1,
    TFT_MOSI,
    TFT_CS
);

tft.initR(INITR_BLACKTAB);
tft.setRotation(1);
```

---

# 🎛️ Button & RGB LED

## Push Button

| Komponen    |   GPIO |
| ----------- | -----: |
| Push Button | GPIO 4 |

Konfigurasi:

```cpp
pinMode(BUTTON_PIN, INPUT_PULLUP);
```

Button bekerja dengan logika:

```text
GPIO 4 ───── Button ───── GND

HIGH = tidak ditekan
LOW  = ditekan
```

---

## RGB LED

| Warna |   GPIO |
| ----- | -----: |
| Red   | GPIO 5 |
| Green | GPIO 6 |
| Blue  | GPIO 7 |

Fungsi indikator:

| Warna      | Status          |
| ---------- | --------------- |
| 🔵 Blue    | LIVE mode       |
| 🔴 Red     | Mengambil PHOTO |
| 🟢 Green   | PHOTO berhasil  |
| 🟡 Yellow  | Decode error    |
| 🟣 Magenta | PHOTO timeout   |

---

# 💡 ESP32-CAM Flash LED

Flash LED menggunakan:

```cpp
#define FLASH_PIN 4
```

Flash digunakan sebagai indikator diagnostik.

### Indikasi

```text
1 kedip setiap byte
      ↓
Data UART masuk

3 kedip cepat
      ↓
STREAM diterima

2 kedip
      ↓
PHOTO diterima
```

Saat startup:

```text
3× kedip
↓
CAM READY
```

---

# 📡 Communication Protocol

ESP32-CAM tidak mengirim JPEG mentah begitu saja.

Setiap frame dibungkus menggunakan format:

```text
┌───────────┬──────┬───────┬─────────────┬──────┐
│ MAGIC 4B  │ TYPE │ LEN 4B│ JPEG PAYLOAD│ CHK  │
└───────────┴──────┴───────┴─────────────┴──────┘
```

### Format Paket

| Field   |     Size | Fungsi             |
| ------- | -------: | ------------------ |
| MAGIC   |   4 byte | Sinkronisasi frame |
| TYPE    |   1 byte | Jenis frame        |
| LEN     |   4 byte | Panjang JPEG       |
| PAYLOAD | LEN byte | Data JPEG          |
| CHK     |   1 byte | XOR checksum       |

---

## MAGIC

Digunakan:

```cpp
const uint8_t MAGIC[4] = {
    0xA5,
    0x5A,
    0xC3,
    0x3C
};
```

Penggunaan 4-byte MAGIC lebih aman dibanding hanya mencari:

```text
FF D8
```

karena:

```text
FF D8
```

merupakan JPEG Start Of Image marker dan dapat muncul sebagai bagian dari data JPEG.

---

# 🏷️ Frame Type

```cpp
#define TYPE_STREAM 0x01
#define TYPE_PHOTO  0x02
```

### STREAM

```text
TYPE = 0x01
```

Digunakan untuk live camera.

### PHOTO

```text
TYPE = 0x02
```

Digunakan untuk snapshot/freeze image.

---

# 🔐 XOR Checksum

Setiap JPEG dihitung checksum sederhana:

```cpp
uint8_t chk = 0;

for (uint32_t i = 0; i < fb->len; i++) {
    chk ^= fb->buf[i];
}
```

Receiver melakukan perhitungan ulang:

```cpp
uint8_t chkCalc = 0;

for (uint32_t i = 0; i < len; i++) {
    chkCalc ^= buf[i];
}
```

Kemudian:

```text
CHK received == CHK calculated
        │
        ├── YES → Frame valid
        │
        └── NO  → Frame dibuang
```

Tujuannya supaya JPEG yang rusak atau terpotong **tidak dipaksa ditampilkan**.

---

# 🕹️ Command Protocol

ESP32-C3 mengirim command melalui UART.

## STREAM

```text
STREAM\n
```

Perintah:

```text
ESP32-C3
   │
   │ STREAM
   ▼
ESP32-CAM
   │
   └── streaming = true
```

---

## PHOTO

```text
PHOTO\n
```

Saat menerima PHOTO:

```text
streaming = false
photoPending = true
```

Streaming dihentikan terlebih dahulu agar paket PHOTO tidak bercampur dengan frame STREAM.

---

## STOP

```text
STOP\n
```

Menghentikan streaming.

---

# 🔄 Operating Mode

## LIVE Mode

```text
ESP32-C3
   │
   │ STREAM
   ▼
ESP32-CAM
   │
   ├── Capture JPEG
   │
   ├── Add MAGIC
   │
   ├── Add TYPE
   │
   ├── Add LENGTH
   │
   ├── Add CHECKSUM
   │
   ▼
UART 460800
   │
   ▼
ESP32-C3
   │
   ├── Find MAGIC
   ├── Read TYPE
   ├── Read LENGTH
   ├── Receive JPEG
   ├── Verify checksum
   │
   ▼
TJpg_Decoder
   │
   ▼
TFT
```

---

# 📸 PHOTO Mode

Ketika tombol C3 ditekan:

```text
LIVE
 │
 ▼
PHOTO command
 │
 ▼
ESP32-CAM stop streaming
 │
 ▼
Capture JPEG
 │
 ▼
Send TYPE_PHOTO
 │
 ▼
ESP32-C3 receive
 │
 ▼
Checksum validation
 │
 ▼
JPEG decode
 │
 ▼
TFT freeze image
```

RGB LED berubah:

```text
🔵 LIVE
 ↓
🔴 CAPTURE
 ↓
🟢 PHOTO OK
```

---

# 🔘 User Interface

### Tombol ditekan saat LIVE

```text
LIVE
 ↓
PHOTO
```

Layar menampilkan hasil snapshot dan gambar berhenti pada frame tersebut.

### Tombol ditekan saat PHOTO

```text
PHOTO
 ↓
LIVE
```

ESP32-C3 mengirim:

```text
STREAM
```

dan ESP32-CAM kembali mengirim frame secara kontinu.

---

# 🖥️ Splash Screen

Saat ESP32-C3 startup, sistem menjalankan splash animation:

```text
Color Sweep
    ↓
Diagonal Wipe
    ↓
CAMDIG
Paceenihh
    ↓
Camera Module v1.0
    ↓
ESP32-C3 + TFT
    ↓
Connecting to CAM...
    ↓
READY
    ↓
Starting live view...
```

---

# 📦 Software

## ESP32-CAM

Library utama:

```cpp
#include "esp_camera.h"
```

## ESP32-C3

Library:

```cpp
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <TJpg_Decoder.h>
```

### Fungsi Library

| Library           | Fungsi         |
| ----------------- | -------------- |
| `esp_camera`      | Camera capture |
| `SPI`             | Komunikasi TFT |
| `Adafruit_GFX`    | Graphics       |
| `Adafruit_ST7735` | TFT driver     |
| `TJpg_Decoder`    | JPEG decoding  |

---

# 🧠 Memory & UART Buffer

ESP32-C3 menggunakan RX buffer:

```cpp
#define UART_RX_BUFFER_SIZE 16384
```

Buffer diperbesar sebelum UART dimulai:

```cpp
Serial1.setRxBufferSize(UART_RX_BUFFER_SIZE);
Serial1.begin(460800, SERIAL_8N1, CAM_RX, CAM_TX);
```

Hal ini diperlukan karena JPEG dikirim dalam ukuran beberapa KB sementara ESP32-C3 juga harus menggambar ke TFT.

Tanpa buffer yang cukup, data UART dapat overflow ketika proses rendering sedang berlangsung.

---

# 🐛 Diagnostic Counter

ESP32-C3 memiliki counter:

```cpp
uint32_t framesOk = 0;
uint32_t framesFail = 0;
```

Serial Monitor menampilkan:

```text
Frame OK: 120 | Frame gagal/timeout: 3
```

Interpretasi:

```text
Frame OK naik
    ↓
Komunikasi berjalan

Frame gagal naik terus
    ↓
Periksa UART / power / baudrate / checksum
```

---

# 🔋 Power Supply

Sumber daya utama:

```text
Li-Po Battery
3.7 V
1000 mAh
```

Charging menggunakan:

```text
USB Type-C
     │
     ▼
┌─────────────┐
│   TP4056    │
│ Li-Po       │
│ Charger     │
└──────┬──────┘
       │
       ▼
Li-Po 3.7 V 1000 mAh
       │
       ▼
CAMDIG System
```

### Komponen

| Komponen      | Spesifikasi |
| ------------- | ----------- |
| Battery       | Li-Po 3.7 V |
| Capacity      | 1000 mAh    |
| Charger       | TP4056      |
| Input charger | USB Type-C  |

> ⚠️ **TP4056 berfungsi sebagai charger Li-Po, bukan sebagai regulator 3.3 V.** Jalur power ke ESP32 harus mengikuti regulator/power circuit yang digunakan pada hardware aktual.

---

# 🧪 Diagnostic Checklist

Jika CAM tidak mengirim gambar:

### 1. Cek Camera

Serial ESP32-CAM:

```text
Camera init OK
CAM READY - diagnostic mode
```

Jika muncul:

```text
Init GAGAL: 0x...
```

periksa camera module dan pin configuration.

---

### 2. Cek UART

Pastikan:

```text
CAM TX → C3 RX
CAM RX → C3 TX
GND    → GND
```

dan:

```text
460800 baud
```

---

### 3. Cek Command

Saat startup C3:

```text
STREAM dikirim ke CAM
```

ESP32-CAM seharusnya menampilkan:

```text
CMD -> STREAM
```

---

### 4. Cek Flash LED

Jika flash ESP32-CAM berkedip ketika command dikirim:

```text
UART → CAM
```

berarti data command masuk ke CAM.

---

### 5. Cek Frame Counter

Contoh normal:

```text
Frame OK: 150 | Frame gagal/timeout: 2
```

Jika:

```text
Frame OK: 0 | Frame gagal/timeout: 100
```

periksa:

* TX/RX
* baudrate
* GND
* UART buffer
* framing MAGIC
* power supply

---

### 6. Jika PHOTO Timeout

Periksa urutan:

```text
C3
 │
 ├── PHOTO
 │
 ▼
CAM
 │
 ├── streaming = false
 ├── capture
 └── TYPE_PHOTO
 │
 ▼
C3
 │
 ├── receive
 ├── checksum
 └── decode
```

---

# 📁 Recommended Project Structure

```text
CAMDIG-Paceenihh/
│
├── README.md
│
├── ESP32-CAM/
│   └── camdig_esp32cam.ino
│
├── ESP32-C3/
│   └── camdig_esp32c3.ino
│
├── docs/
│   ├── wiring.md
│   ├── protocol.md
│   └── troubleshooting.md
│
├── hardware/
│   ├── wiring/
│   ├── schematic/
│   └── photos/
│
└── media/
    ├── splash/
    └── demo/
```

---

# 📌 Pin Summary

## ESP32-CAM

| Fungsi    |                                                 GPIO |
| --------- | ---------------------------------------------------: |
| Camera    | GPIO 0, 5, 18, 19, 21, 22, 23, 25, 26, 27, 32, 34–39 |
| UART TX   |                                               GPIO 1 |
| UART RX   |                                               GPIO 3 |
| Flash LED |                                               GPIO 4 |
| Button    |                                              GPIO 13 |

## ESP32-C3

| Fungsi    |    GPIO |
| --------- | ------: |
| UART RX   | GPIO 20 |
| UART TX   | GPIO 21 |
| TFT SCLK  |  GPIO 8 |
| TFT MOSI  | GPIO 10 |
| TFT CS    |  GPIO 1 |
| TFT RST   |  GPIO 3 |
| TFT DC    |  GPIO 2 |
| Button    |  GPIO 4 |
| RGB Red   |  GPIO 5 |
| RGB Green |  GPIO 6 |
| RGB Blue  |  GPIO 7 |

---

# ⚠️ Important Notes

### UART

```text
Baudrate = 460800
```

Jangan mengubah baudrate hanya pada salah satu board.

---

### JPEG

Format:

```text
JPEG / QVGA / 320×240
```

Frame dikirim dengan custom packet protocol.

---

### PHOTO

PHOTO selalu menghentikan streaming terlebih dahulu.

Hal ini mencegah:

```text
STREAM FRAME
STREAM FRAME
PHOTO FRAME
STREAM FRAME
```

bercampur di UART.

---

### Checksum

Checksum hanya menggunakan:

```text
XOR
```

Ini merupakan validasi sederhana untuk mendeteksi frame rusak, bukan mekanisme error correction.

---

# 🚧 Known Limitations

* UART masih menggunakan komunikasi point-to-point.
* XOR checksum belum memiliki mekanisme retransmission.
* Jika frame rusak, frame langsung dibuang.
* Rendering TFT dapat mempengaruhi timing penerimaan UART.
* Live streaming belum menggunakan compression tambahan selain JPEG.
* Belum menggunakan wireless protocol seperti ESP-NOW.

---

# 🔮 Future Development

Beberapa pengembangan yang memungkinkan:

* [ ] ESP-NOW communication
* [ ] Wireless camera streaming
* [ ] JPEG retransmission
* [ ] CRC-16 / CRC-32
* [ ] Frame sequence number
* [ ] FPS counter
* [ ] Battery voltage monitor
* [ ] Battery percentage indicator
* [ ] Camera settings menu
* [ ] Adjustable JPEG quality
* [ ] Adjustable resolution
* [ ] SD card image storage
* [ ] Wi-Fi image transfer
* [ ] Web-based camera viewer

---

# 📸 Project Identity

```text
╔══════════════════════════════════╗
║          CAMDIG PACEENIHH        ║
║                                  ║
║      ESP32-CAM + ESP32-C3        ║
║             + TFT                ║
║                                  ║
║        Camera Module v1.0        ║
╚══════════════════════════════════╝
```

**Project:** CAMDIG Paceenihh
**Platform:** ESP32-CAM + ESP32-C3
**Display:** TFT ST7735
**Communication:** UART 460800
**Image Format:** JPEG
**Resolution:** QVGA 320×240
**Power:** Li-Po 3.7 V 1000 mAh
**Charger:** TP4056 USB Type-C

---

## 👨‍💻 Author

**Ficram Manifur Farissa**

Embedded Systems • IoT • Edge AI • Robotics

---

## 📊 Project Status

```text
Camera Capture       ✅
JPEG Streaming       ✅
UART Communication   ✅
Packet Framing       ✅
Checksum Validation  ✅
TFT Display          ✅
Live Mode            ✅
Photo Mode           ✅
RGB Status           ✅
Splash Screen        ✅
Battery Powered      ✅
```

> **CAMDIG Paceenihh v1.0 — Functional Prototype**
