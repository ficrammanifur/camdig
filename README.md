<div id="top"></div>

<h1 align="center">📷 CAMDIG Paceenihh</h1>

<p align="center">
  <strong>Camera Digital Module • ESP32-CAM + ESP32-C3 + TFT</strong>
</p>

<p align="center">
  <a href="https://www.espressif.com/en/products/socs/esp32">
    <img src="https://img.shields.io/badge/ESP32--CAM-000000?style=for-the-badge&logo=espressif&logoColor=white" alt="ESP32-CAM">
  </a>
  <a href="https://www.espressif.com/en/products/socs/esp32-c3">
    <img src="https://img.shields.io/badge/ESP32--C3-000000?style=for-the-badge&logo=espressif&logoColor=white" alt="ESP32-C3">
  </a>
  <a href="https://www.arduino.cc/">
    <img src="https://img.shields.io/badge/Arduino-00979D?style=for-the-badge&logo=arduino&logoColor=white" alt="Arduino">
  </a>
  <a href="https://github.com/Bodmer/TJpg_Decoder">
    <img src="https://img.shields.io/badge/TJpg--Decoder-Image%20Decode-2563eb?style=for-the-badge" alt="TJpg Decoder">
  </a>
  <a href="https://github.com/adafruit/Adafruit-ST7735-Library">
    <img src="https://img.shields.io/badge/ST7735-TFT-7c3aed?style=for-the-badge" alt="ST7735">
  </a>
  <a href="https://github.com/ficrammanifur">
    <img src="https://img.shields.io/badge/Embedded-IoT-059669?style=for-the-badge" alt="Embedded IoT">
  </a>
  <a href="LICENSE">
    <img src="https://img.shields.io/badge/License-MIT-blue?style=for-the-badge" alt="License">
  </a>
</p>

<p align="center">
  <a href="#-overview">Overview</a> •
  <a href="#-architecture">Architecture</a> •
  <a href="#-hardware">Hardware</a> •
  <a href="#-communication-protocol">Protocol</a> •
  <a href="#-software">Software</a> •
  <a href="#-troubleshooting">Troubleshooting</a>
</p>

---

## 📸 Overview

**CAMDIG Paceenihh** adalah modul kamera digital berbasis **ESP32-CAM AI Thinker** yang dikombinasikan dengan **ESP32-C3** sebagai controller dan display interface.

ESP32-CAM bertugas mengambil gambar menggunakan kamera OV2640, kemudian mengirimkan JPEG melalui **UART 460800 baud** menuju ESP32-C3.

ESP32-C3 menerima data, melakukan validasi frame menggunakan **MAGIC + LENGTH + XOR CHECKSUM**, kemudian mendecode gambar menggunakan `TJpg_Decoder` dan menampilkannya pada TFT ST7735.

### ✨ Main Features

| Feature                    | Status |
| -------------------------- | :----: |
| 📷 Camera Capture          |    ✅   |
| 🎥 Live Streaming          |    ✅   |
| 🖼️ Snapshot / Photo Mode  |    ✅   |
| 📺 TFT Display             |    ✅   |
| 🔄 LIVE ↔ PHOTO            |    ✅   |
| 📡 UART 460800             |    ✅   |
| 🔐 Packet Framing          |    ✅   |
| ✔️ XOR Checksum            |    ✅   |
| 🌈 RGB Status Indicator    |    ✅   |
| 💡 Camera Flash Diagnostic |    ✅   |
| 🎬 Splash Screen           |    ✅   |
| 🔋 Battery Powered         |    ✅   |

---

## 🧭 Project Flow

```text
                    ┌──────────────────────┐
                    │    Li-Po 3.7V        │
                    │      1000mAh         │
                    └──────────┬───────────┘
                               │
                         ┌─────▼─────┐
                         │   TP4056  │
                         │ USB Type-C│
                         └─────┬─────┘
                               │
              ┌────────────────┴────────────────┐
              │                                 │
       ┌──────▼──────┐                   ┌──────▼──────┐
       │  ESP32-CAM  │                   │  ESP32-C3   │
       │  AI Thinker │                   │  Controller  │
       │   OV2640    │                   │              │
       └──────┬──────┘                   └──────┬──────┘
              │                                 │
              │ UART 460800                     │ SPI
              │                                 │
              └────────────►──────────────┐     │
                       JPEG DATA           │     │
                                           ▼     ▼
                                      ┌─────────────┐
                                      │ TFT ST7735  │
                                      │ 320 × 240*  │
                                      └─────────────┘
```

> `*` Resolusi camera menggunakan QVGA 320×240. Resolusi fisik TFT mengikuti modul ST7735 yang digunakan.

---

# 🧩 Architecture

```text
┌─────────────────────────────────────────────────────────┐
│                     CAMDIG Paceenihh                    │
├─────────────────────────────────────────────────────────┤
│                                                         │
│   CAMERA SIDE                    DISPLAY SIDE           │
│                                                         │
│  ┌──────────────┐              ┌──────────────┐         │
│  │  ESP32-CAM   │              │   ESP32-C3   │         │
│  │              │              │              │         │
│  │   OV2640     │              │ UART RX      │         │
│  │      │       │              │      │       │         │
│  │      ▼       │              │      ▼       │         │
│  │ JPEG Capture │              │ Packet Parser│         │
│  │      │       │              │      │       │         │
│  │      ▼       │              │      ▼       │         │
│  │ Packet Frame ├──── UART ───►│ Checksum     │         │
│  │              │   460800     │      │       │         │
│  └──────────────┘              │      ▼       │         │
│                                │ JPEG Decoder  │         │
│                                │      │       │         │
│                                │      ▼       │         │
│                                │ TFT ST7735   │         │
│                                └──────────────┘         │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

---

# 🔌 Hardware

## 📷 ESP32-CAM AI Thinker

### Camera Pin Configuration

| Function | GPIO |
| -------- | ---: |
| PWDN     |   32 |
| RESET    |   -1 |
| XCLK     |    0 |
| SIOD     |   26 |
| SIOC     |   27 |
| Y9       |   35 |
| Y8       |   34 |
| Y7       |   39 |
| Y6       |   36 |
| Y5       |   21 |
| Y4       |   19 |
| Y3       |   18 |
| Y2       |    5 |
| VSYNC    |   25 |
| HREF     |   23 |
| PCLK     |   22 |

### Camera Configuration

```cpp
pixel_format = PIXFORMAT_JPEG
frame_size   = FRAMESIZE_QVGA
jpeg_quality = 20
fb_count     = 2
grab_mode    = CAMERA_GRAB_LATEST
```

```text
Resolution : 320 × 240
Format     : JPEG
Quality    : 20
Frame Buff : 2
```

---

# 📺 ESP32-C3

ESP32-C3 berfungsi sebagai **main controller + image receiver + display controller**.

### TFT ST7735

| TFT Pin | ESP32-C3 |
| ------- | -------: |
| SCLK    |   GPIO 8 |
| MOSI    |  GPIO 10 |
| CS      |   GPIO 1 |
| RST     |   GPIO 3 |
| DC      |   GPIO 2 |

### UART

| ESP32-C3 | Function |
| -------- | -------- |
| GPIO 20  | RX       |
| GPIO 21  | TX       |

### User Interface

| Component   |   GPIO |
| ----------- | -----: |
| Push Button | GPIO 4 |
| RGB Red     | GPIO 5 |
| RGB Green   | GPIO 6 |
| RGB Blue    | GPIO 7 |

---

# 🔗 UART Wiring

```text
        ESP32-CAM                     ESP32-C3
       ┌───────────┐                ┌───────────┐
       │           │                │           │
       │ GPIO 1 TX ├───────────────►│ GPIO 20 RX│
       │           │                │           │
       │ GPIO 3 RX │◄───────────────┤ GPIO 21 TX│
       │           │                │           │
       │ GND       ├────────────────┤ GND       │
       │           │                │           │
       └───────────┘                └───────────┘

                  UART = 460800 8N1
```

> ⚠️ TX → RX dan RX → TX.

---

# 🔘 Button

```text
ESP32-C3 GPIO 4
       │
       │
    ┌──┴──┐
    │     │
  BUTTON  │
    │     │
    └──┬──┘
       │
      GND
```

Configuration:

```cpp
pinMode(BUTTON_PIN, INPUT_PULLUP);
```

| State    | GPIO |
| -------- | ---- |
| Released | HIGH |
| Pressed  | LOW  |

---

# 🌈 RGB Status

| LED        | Status          |
| ---------- | --------------- |
| 🔵 Blue    | LIVE            |
| 🔴 Red     | Capturing PHOTO |
| 🟢 Green   | PHOTO OK        |
| 🟡 Yellow  | Decode Error    |
| 🟣 Magenta | PHOTO Timeout   |

---

# 💡 Camera Flash Diagnostic

ESP32-CAM menggunakan:

```cpp
#define FLASH_PIN 4
```

Flash digunakan sebagai **visual UART diagnostic**.

```text
UART byte received
        │
        ▼
 Flash ON 30ms
        │
        ▼
 Flash OFF
```

Command valid:

```text
STREAM → 3× blink
PHOTO  → 2× blink
```

---

# 🔋 Power Supply

CAMDIG menggunakan battery portable:

```text
┌──────────────────┐
│ Li-Po Battery    │
│ 3.7V / 1000mAh   │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ TP4056 USB Type-C│
│ Li-Po Charger    │
└────────┬─────────┘
         │
         ▼
    CAMDIG System
```

### Power Components

| Component     | Specification |
| ------------- | ------------- |
| Battery       | Li-Po         |
| Voltage       | 3.7 V nominal |
| Capacity      | 1000 mAh      |
| Charger       | TP4056        |
| Charging Port | USB Type-C    |

> ⚠️ TP4056 berfungsi sebagai **charger Li-Po**, bukan regulator 3.3 V. Jalur regulator mengikuti rangkaian power hardware yang digunakan pada prototype.

---

# 📡 Communication Protocol

CAMDIG menggunakan custom packet framing untuk mengirim JPEG.

## Packet Structure

```text
┌──────────┬──────┬────────┬─────────────────┬──────┐
│ MAGIC 4B │ TYPE │ LEN 4B │ JPEG PAYLOAD    │ CHK  │
└──────────┴──────┴────────┴─────────────────┴──────┘
```

### Packet Field

| Field   |     Size | Description           |
| ------- | -------: | --------------------- |
| MAGIC   |   4 byte | Frame synchronization |
| TYPE    |   1 byte | STREAM / PHOTO        |
| LEN     |   4 byte | JPEG payload length   |
| PAYLOAD | Variable | JPEG data             |
| CHK     |   1 byte | XOR checksum          |

---

# 🧲 MAGIC

```cpp
const uint8_t MAGIC[4] = {
    0xA5,
    0x5A,
    0xC3,
    0x3C
};
```

Menggunakan 4-byte magic membuat sinkronisasi lebih aman dibanding hanya mencari:

```text
FF D8
```

karena `FF D8` merupakan JPEG Start Of Image marker.

---

# 🏷️ Frame Type

```cpp
#define TYPE_STREAM 0x01
#define TYPE_PHOTO  0x02
```

| Type   |  Value | Function   |
| ------ | -----: | ---------- |
| STREAM | `0x01` | Live frame |
| PHOTO  | `0x02` | Snapshot   |

---

# 🔐 XOR Checksum

Checksum dihitung dari seluruh JPEG payload:

```cpp
uint8_t chk = 0;

for (uint32_t i = 0; i < fb->len; i++) {
    chk ^= fb->buf[i];
}
```

Receiver menghitung kembali checksum.

```text
       JPEG
        │
        ▼
   XOR checksum
        │
        ▼
┌─────────────────┐
│ Received == Calc │
└────────┬────────┘
         │
      ┌──┴──┐
      │     │
     YES    NO
      │     │
      ▼     ▼
   Decode  Discard
```

> Checksum digunakan untuk **deteksi frame rusak**, bukan untuk memperbaiki data yang rusak.

---

# 🕹️ Command Protocol

ESP32-C3 mengirim command melalui UART.

### STREAM

```text
STREAM\n
```

Mengaktifkan live streaming.

### PHOTO

```text
PHOTO\n
```

Menghentikan streaming dan meminta snapshot.

### STOP

```text
STOP\n
```

Menghentikan streaming.

---

# 🔄 LIVE Mode

```text
┌──────────────┐
│ ESP32-C3     │
└──────┬───────┘
       │ STREAM
       ▼
┌──────────────┐
│ ESP32-CAM    │
└──────┬───────┘
       │
       ▼
 Capture JPEG
       │
       ▼
 Add Packet Header
       │
       ▼
 XOR Checksum
       │
       ▼
 UART 460800
       │
       ▼
┌──────────────┐
│ ESP32-C3     │
│              │
│ Find MAGIC   │
│ Read LENGTH  │
│ Read JPEG    │
│ Check CHK    │
└──────┬───────┘
       │
       ▼
 TJpg_Decoder
       │
       ▼
    TFT LIVE
```

---

# 📸 PHOTO Mode

Ketika tombol ditekan:

```text
LIVE
 │
 │ Button
 ▼
PHOTO command
 │
 ▼
CAM stop streaming
 │
 ▼
Capture JPEG
 │
 ▼
TYPE_PHOTO
 │
 ▼
UART
 │
 ▼
C3 Receive
 │
 ▼
Checksum
 │
 ▼
Decode
 │
 ▼
TFT Freeze
```

Setelah PHOTO berhasil:

```text
🔴 Capturing
      ↓
🟢 PHOTO OK
```

---

# 🔁 Mode Switching

```text
             ┌──────────────┐
             │              │
             ▼              │
        ┌─────────┐         │
        │  LIVE   │         │
        └────┬────┘         │
             │              │
        Button Press        │
             │              │
             ▼              │
        ┌─────────┐         │
        │  PHOTO  │         │
        └────┬────┘         │
             │              │
        Button Press        │
             │              │
             └──────────────┘
```

---

# 🎬 Splash Screen

ESP32-C3 menampilkan splash animation saat startup.

```text
Color Sweep
     ↓
Diagonal Wipe
     ↓
┌─────────────────┐
│     CAMDIG      │
│   Paceenihh     │
│                 │
│ Camera Module   │
│     v1.0        │
└─────────────────┘
     ↓
Connecting to CAM...
     ↓
READY
     ↓
Starting live view...
```

---

# 💻 Software

## ESP32-CAM

```cpp
#include "esp_camera.h"
```

## ESP32-C3

```cpp
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <TJpg_Decoder.h>
```

### Software Stack

```text
┌────────────────────────────┐
│       CAMDIG Application   │
├────────────────────────────┤
│ JPEG Packet Protocol       │
├────────────────────────────┤
│ UART 460800                │
├────────────────────────────┤
│ ESP32 Arduino Framework    │
├────────────────────────────┤
│ ESP32-CAM / ESP32-C3       │
└────────────────────────────┘
```

---

# 📦 UART Buffer

ESP32-C3 menggunakan RX buffer sebesar:

```cpp
#define UART_RX_BUFFER_SIZE 16384
```

Buffer harus diset sebelum UART dimulai:

```cpp
Serial1.setRxBufferSize(UART_RX_BUFFER_SIZE);

Serial1.begin(
    460800,
    SERIAL_8N1,
    CAM_RX,
    CAM_TX
);
```

Tujuannya mengurangi risiko **UART overflow** ketika ESP32-C3 sedang sibuk menggambar JPEG ke TFT.

---

# 📊 Diagnostic Monitor

ESP32-C3 menyediakan counter:

```text
Frame OK: 150 | Frame gagal/timeout: 3
```

### Interpretation

| Output                        | Meaning                 |
| ----------------------------- | ----------------------- |
| `Frame OK` meningkat          | Frame berhasil diterima |
| `Frame gagal` sedikit         | Normal                  |
| `Frame gagal` terus meningkat | Periksa komunikasi      |

---

# 🛠️ Troubleshooting

<details>
<summary><strong>❌ Camera init gagal</strong></summary>

Periksa:

* Camera ribbon cable
* AI Thinker pin configuration
* Power ESP32-CAM
* OV2640
* PSRAM

Serial Monitor harus menunjukkan:

```text
Camera init OK
CAM READY - diagnostic mode
```

</details>

<details>
<summary><strong>❌ CAM tidak menerima command</strong></summary>

Periksa UART:

```text
CAM TX → C3 RX
CAM RX → C3 TX
GND    → GND
```

Baudrate:

```text
460800
```

Jika Flash LED CAM berkedip ketika command dikirim, berarti data UART sudah masuk ke CAM.

</details>

<details>
<summary><strong>❌ TFT tetap hitam</strong></summary>

Periksa:

* TFT SCLK
* TFT MOSI
* CS
* DC
* RST
* SPI configuration
* `INITR_BLACKTAB`
* `setRotation(1)`

</details>

<details>
<summary><strong>❌ Frame timeout terus</strong></summary>

Periksa:

```text
UART wiring
    ↓
Baudrate
    ↓
RX buffer
    ↓
MAGIC
    ↓
Packet LENGTH
    ↓
Checksum
```

</details>

<details>
<summary><strong>❌ PHOTO timeout</strong></summary>

Pastikan urutan:

```text
C3 → PHOTO
CAM → Stop STREAM
CAM → Capture
CAM → TYPE_PHOTO
CAM → JPEG
C3 → Checksum
C3 → Decode
C3 → TFT
```

</details>

---

# 📁 Project Structure

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

# 📌 Pin Reference

### ESP32-CAM

| Function  | GPIO |
| --------- | ---: |
| UART TX   |  `1` |
| UART RX   |  `3` |
| Flash LED |  `4` |
| Button    | `13` |

### ESP32-C3

| Function  | GPIO |
| --------- | ---: |
| UART RX   | `20` |
| UART TX   | `21` |
| TFT SCLK  |  `8` |
| TFT MOSI  | `10` |
| TFT CS    |  `1` |
| TFT RST   |  `3` |
| TFT DC    |  `2` |
| Button    |  `4` |
| RGB Red   |  `5` |
| RGB Green |  `6` |
| RGB Blue  |  `7` |

---

# 📋 Technical Summary

| Parameter         | Value                |
| ----------------- | -------------------- |
| Camera            | AI Thinker ESP32-CAM |
| Sensor            | OV2640               |
| Controller        | ESP32-C3             |
| Display           | ST7735 TFT           |
| Image             | JPEG                 |
| Camera Resolution | QVGA 320×240         |
| UART              | 460800 baud          |
| Packet Sync       | 4-byte MAGIC         |
| Error Detection   | XOR checksum         |
| JPEG Decoder      | TJpg_Decoder         |
| Battery           | Li-Po 3.7 V 1000 mAh |
| Charger           | TP4056 USB Type-C    |
| Operating Mode    | LIVE / PHOTO         |

---

# 🚧 Known Limitations

* UART masih menggunakan komunikasi wired point-to-point.
* XOR checksum hanya mendeteksi error.
* Belum terdapat retransmission frame.
* Frame yang rusak langsung dibuang.
* Rendering TFT dapat mempengaruhi timing penerimaan UART.
* Live stream belum memiliki FPS control khusus.
* Belum ada battery monitoring.
* Belum menggunakan ESP-NOW.

---

# 🚀 Future Development

* [ ] ESP-NOW wireless camera
* [ ] CRC-16 / CRC-32
* [ ] Frame sequence number
* [ ] Automatic frame retry
* [ ] FPS counter
* [ ] Battery voltage monitoring
* [ ] Battery percentage
* [ ] Camera configuration menu
* [ ] Adjustable JPEG quality
* [ ] Adjustable resolution
* [ ] SD Card storage
* [ ] Wi-Fi image transfer
* [ ] Web camera viewer

---

# 📷 Project Identity

<div align="center">

### CAMDIG Paceenihh

**Camera Module v1.0**

```text
ESP32-CAM
    │
    │ UART 460800
    ▼
 ESP32-C3
    │
    │ SPI
    ▼
 TFT ST7735
```

**LIVE • PHOTO • SNAPSHOT • JPEG STREAM**

</div>

---

## 👨‍💻 Author

<div align="center">

**Ficram Manifur Farissa**

Embedded Systems • IoT • Edge AI • Robotics

</div>

---

## ⭐ Project Status

<div align="center">

| Module                |   Status   |
| --------------------- | :--------: |
| 📷 Camera Capture     | 🟢 Working |
| 📡 UART Communication | 🟢 Working |
| 📦 Packet Framing     | 🟢 Working |
| 🔐 Checksum           | 🟢 Working |
| 🎥 Live Streaming     | 🟢 Working |
| 📸 Photo Mode         | 🟢 Working |
| 📺 TFT Display        | 🟢 Working |
| 🌈 RGB Indicator      | 🟢 Working |
| 🎬 Splash Screen      | 🟢 Working |
| 🔋 Battery Power      | 🟢 Working |

<br>

**⚡ Built with ESP32**

**📷 Built for CAMDIG Paceenihh**

**⭐ Star this repo if you like it!**

<p><a href="#top">⬆ Kembali ke Atas</a></p>

</div>

---

<div align="center">

<sub>CAMDIG Paceenihh • Camera Digital Module • v1.0</sub>

</div>
