# SecureDrop

Smart Parcel Security Box — STEAM competition MVP (v1)

Indoor demo prototype: barcode unlock → wait → capture courier photo → Telegram notify → relock.

---

## v1 Architecture

```
┌─────────────────────────┐         HTTP GET /capture        ┌─────────────────────┐
│  Main Controller        │ ───────────────────────────────► │  ESP32-CAM          │
│  ESP32-WROOM-32         │ ◄──────────── JPEG ───────────── │  AI-Thinker         │
│                         │                                  │  Fixed IP           │
│  • UART barcode (GM65)  │                                  │  Front of box       │
│  • Relay → 12V solenoid │                                  └─────────────────────┘
│  • Local NVS database   │
│  • Telegram Bot HTTPS   │ ───────► Telegram (text + photo)
│  • FSM orchestrator     │
└─────────────────────────┘
```

**Not in v1 (architecture reserved only):** UV, ultrasonic, load cell, anti-theft sensors, Firebase, OLED, second camera, buzzer/siren.

### Delivery flow

1. Boot → init lock (LOCKED) → WiFi → Telegram client → wait for barcode  
2. Read tracking number (UART 9600, RX-only)  
3. Verify against **local NVS database** (no Firebase / Sheets)  
4. **Valid:** mark used → unlock 5 s → HTTP capture → Telegram (tracking + time + photo) → lock → idle  
5. **Invalid / already used:** do not unlock → Telegram alert → idle  

### State machine

`BOOT → CONNECT_WIFI → READY → WAIT_BARCODE → VERIFY → VALID → UNLOCK → CAPTURE → SEND_TELEGRAM → LOCK → READY`  

Invalid path: `VERIFY → ERROR → READY`

---

## Hardware (v1 only)

| Item | Role |
|---|---|
| ESP32 DevKit V1 (WROOM-32) | Main controller |
| ESP32-CAM AI-Thinker | Courier photo via HTTP |
| GM65-compatible UART scanner | Tracking number input |
| 12V Fail-Secure solenoid | Lock |
| 1-channel 5V relay (active-LOW) | Drives solenoid |
| 12V adapter + LM2596 → 5V | Shared logic power |
| Common GND | All modules |

### Chosen pin map (Main)

| Function | GPIO | Notes |
|---|---|---|
| Barcode RX | **16** | UART2, scanner TX → ESP32 RX |
| Barcode TX | not used | RX-only (`-1`) |
| Relay IN | **25** | Active-LOW → energize = unlock |

### Power

```
12V Adapter ──► Solenoid (via Relay COM/NO)
            └─► LM2596 5V ──► ESP32 Main VIN/5V
                            └─► ESP32-CAM 5V
Common GND tied together.
```

Put a flyback diode across the solenoid coil. Do not power the solenoid from the ESP32 5V pin.

### Network defaults

| Device | Address |
|---|---|
| ESP32-CAM | `192.168.1.101` (static) |
| Capture URL | `http://192.168.1.101/capture` |
| Gateway / mask | `192.168.1.1` / `255.255.255.0` |

Change these in `firmware/main_controller/include/config.h` and `firmware/esp32cam/include/credentials.h` if your LAN differs.

### Seed tracking numbers

- `TH1234567890`
- `TH9988776655`
- `JT5566778899`

Used flags persist in NVS across reboots. To reset for demo, erase flash (`pio run -t erase`) or clear the `securedrop` NVS namespace.

---

## Folder structure

```
SecureDrop/
├── README.md
├── docs/                          # original full-scope design notes
└── firmware/
    ├── main_controller/           # PlatformIO — Main ESP32
    │   ├── platformio.ini
    │   ├── include/
    │   │   ├── config.h
    │   │   ├── pins.h
    │   │   └── credentials.h.example
    │   └── src/
    │       ├── main.cpp
    │       ├── state_machine.*
    │       ├── barcode.*
    │       ├── camera.*
    │       ├── telegram.*
    │       ├── database.*
    │       ├── lock.*
    │       └── wifi_manager.*
    └── esp32cam/                  # PlatformIO — one front camera
        ├── platformio.ini
        ├── include/
        │   ├── camera_pins.h
        │   └── credentials.h.example
        └── src/
            └── main.cpp
```

---

## Build & flash

### 1. Prerequisites

- VS Code + [PlatformIO](https://platformio.org/)
- USB cable for Main ESP32
- FTDI / USB-TTL (3.3 V) for ESP32-CAM programming

### 2. Credentials

```bash
cp firmware/main_controller/include/credentials.h.example firmware/main_controller/include/credentials.h
cp firmware/esp32cam/include/credentials.h.example          firmware/esp32cam/include/credentials.h
```

Edit both files: WiFi SSID/password, Telegram bot token + chat id.  
On the camera file, set static IP/gateway to match your router and `CAM_HOST` in Main `config.h`.

### 3. Flash ESP32-CAM

1. GPIO0 → GND (flash mode)  
2. Open `firmware/esp32cam/` in PlatformIO  
3. Build & Upload  
4. Disconnect GPIO0 from GND, press RESET  
5. Serial Monitor `115200` — confirm IP `192.168.1.101`  
6. Browser test: `http://192.168.1.101/capture` should return a JPEG  

### 4. Flash Main Controller

1. Open `firmware/main_controller/`  
2. Build & Upload  
3. Serial Monitor `115200` — wait until FSM reaches `WAIT_BARCODE`  

### 5. Wiring checklist

- Scanner TX → ESP32 GPIO16, scanner VCC 5V, common GND  
- Relay IN → GPIO25, relay VCC 5V, GND common  
- Relay COM → 12V+, NO → solenoid+, solenoid− → 12V GND  
- ESP32-CAM on same WiFi as Main  

---

## Module responsibilities

| Module | Responsibility |
|---|---|
| `wifi_manager` | Connect / reconnect WiFi |
| `barcode` | Non-blocking UART line reader |
| `database` | Local tracking list + NVS used flags |
| `lock` | Fail-secure relay control |
| `camera` | HTTP GET JPEG from CAM |
| `telegram` | `sendMessage` + multipart `sendPhoto` |
| `state_machine` | Entire delivery sequence |

---

## Future expansion (not implemented)

Keep these as separate modules later — do not fold into v1:

| Feature | Suggested module |
|---|---|
| UV sterilization | `uv` + hardware door interlock |
| Anti-theft | `sensor` (MPU6050/SW-420) + `alarm` |
| Ultrasonic / load cell | parcel presence fusion |
| OLED | `display` |
| Firebase | `cloud_db` replacing local verify path |
| Second camera | extra `camera` target + FSM step |

---

*SecureDrop v1 — reliable MVP first, expand later.*
