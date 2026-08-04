# SecureDrop

Smart Parcel Security Box — STEAM competition MVP (v1.1)

Indoor demo: courier scans a registered tracking barcode → solenoid unlocks → ESP32-CAM captures courier → Telegram notify → tracking marked used → lock again.

Invalid or already-used tracking numbers never unlock the box.

---

## Architecture

```
┌─────────────────────────┐         HTTP GET /capture        ┌─────────────────────┐
│  Main Controller        │ ───────────────────────────────► │  ESP32-CAM          │
│  ESP32-WROOM-32         │ ◄──────────── JPEG ───────────── │  AI-Thinker         │
│                         │                                  │  Fixed IP           │
│  • UART barcode         │                                  │  Front of box       │
│  • Relay → 12V solenoid │                                  └─────────────────────┘
│  • Local NVS database   │
│  • Telegram Bot HTTPS   │ ───────► Telegram (text + photo)
│  • FSM orchestrator     │
└─────────────────────────┘
```

**Not in v1:** UV-C, ultrasonic, load cell, anti-theft sensors, Firebase, OLED, second camera, buzzer/siren, CAM motion-detection trigger.

### Delivery flow

1. Boot → lock (LOCKED) → WiFi → wait for barcode  
2. Read tracking number over UART  
3. Verify against **local NVS database**  
4. **Valid:** unlock 5 s → HTTP capture → Telegram → **mark used** → lock → idle  
5. **Invalid / already used:** do not unlock → Telegram alert → idle  

Tracking is marked used only after the unlock → capture → notify path reaches `LOCK` (box was opened). Failures after unlock still consume the code so it cannot be reused.

### State machine

`BOOT → CONNECT_WIFI → READY → WAIT_BARCODE → VERIFY → VALID → UNLOCK → CAPTURE → SEND_TELEGRAM → LOCK → READY`

Invalid path: `VERIFY → ERROR → READY` (lock forced, code not marked used)

---

## Hardware list (v1)

| Item | Role |
|---|---|
| ESP32-WROOM-32 DevKit | Main controller |
| ESP32-CAM (AI-Thinker) | Courier photo via HTTP only |
| MH-ET LIVE / GM65-class UART scanner | Tracking input (confirm exact model) |
| 12V fail-secure solenoid lock | Locked when unpowered |
| 1-channel 5V relay module | Switches solenoid 12V |
| Adjustable DC-DC buck (LM2596 / ZX-052 class) | 12V → regulated 5.0V |
| 12V power adapter | Solenoid branch + buck input |
| Jumper wires / breadboard | Interconnect |

---

## Power architecture

```
12V Adapter
   ├─► Relay COM/NO ──► Solenoid (+)     ← high-current path only
   │                    Solenoid (−) ──► 12V GND
   │                    (+ flyback diode across coil)
   │
   └─► Buck IN ──► OUT 5.0V ──► ESP32-WROOM 5V/VIN
                              ├─► ESP32-CAM 5V
                              └─► Scanner VCC (only if datasheet allows 5V)

Common GND: tie 12V adapter GND, buck GND, ESP32 GND, CAM GND, scanner GND,
and relay logic GND together.
```

Rules:

- Never route solenoid current through the ESP32 or the buck **output**.
- Relay contacts switch the solenoid’s 12V supply (COM/NO for fail-secure: open = locked).
- Default relay GPIO state must keep the solenoid **locked** at boot/reset.
- Set the buck display/pot to **5.0V** before connecting logic boards.
- Scanner: power at 5V only if the module marking/datasheet confirms it (MH-ET LIVE boards are commonly 5V UART modules — verify yours).

### Relay / common ground notes

| Relay module type | Wiring note |
|---|---|
| Opto-isolated, JD-VCC jumper **on** (logic & coil share 5V) | Relay VCC + IN + GND from ESP32 5V domain; common GND with ESP32 required |
| Opto-isolated, JD-VCC jumper **removed** | JD-VCC = relay coil supply; VCC = opto logic; GND still needs a defined return — follow module silk |
| Non-isolated transistor module | Relay VCC/GND must share GND with ESP32; IN is GPIO |

This project assumes a typical **active-LOW** 5V relay module (`RELAY_ACTIVE_LOW = true` in `pins.h`).

---

## Pin map (Main Controller)

| Function | GPIO | Notes |
|---|---|---|
| Barcode RX | **16** | UART2 — scanner TX → ESP32 RX |
| Barcode TX | **17** | Used only in `UART_TRIGGER` mode |
| Barcode TRIG | **-1** (unset) | Set in `pins.h` only if module has TRIG |
| Relay IN | **25** | Active-LOW → energize = unlock |

Do not reassign these until you confirm no breadboard conflict with your harness.

---

## Barcode scan modes

Configured in `firmware/main_controller/include/config.h` via `BARCODE_SCAN_MODE`.

| Mode | Illumination / decode | Firmware behavior | Requirements |
|---|---|---|---|
| `Continuous` | Usually always on | Always listening while armed | None |
| `Presentation` **(default)** | On present only (if module configured) | Passive UART listen + cooldown/dedup | Configure scanner with **manufacturer setup barcodes** |
| `UartTrigger` | On start-cmd / off stop-cmd | Sends UART start/stop, scan timeout | TX wire + **confirmed** command bytes in `barcode_commands.h` |
| `HardwareTrigger` | While TRIG asserted | Asserts `PIN_BARCODE_TRIGGER` | Confirmed TRIG pin + polarity |

### Scanner driver states (non-blocking)

`IDLE → ARMING → SCANNING → CODE_RECEIVED`  
also: `TIMEOUT`, `ERROR`

Timing knobs in `config.h`:

- `BARCODE_SCAN_TIMEOUT_MS` — arm window for UART/hardware trigger  
- `BARCODE_COOLDOWN_MS` — pause after a scan / timeout  
- `BARCODE_DUPLICATE_MS` — ignore the same code within this window  

### Important: no invented UART commands

`barcode_commands.h` ships with **empty** start/stop placeholders.  
`UART_TRIGGER` will refuse to arm until you fill confirmed bytes for your exact model.

Photos/docs in this repo point to an **MH-ET LIVE** UART module and older notes mentioning **GM65**. Confirm the sticker/PCB revision and datasheet before enabling UART or hardware trigger.

### Preferred activation order

1. **Presentation / induction** on the scanner (setup QR/barcodes from the manual) + firmware `Presentation`  
2. UART command trigger — after protocol confirmed  
3. Hardware TRIG pin — after pin/polarity confirmed  
4. ESP32-CAM motion HTTP event — **not implemented**; only if 1–3 are impossible  

---

## Configuration

### 1. Credentials

```bash
cp firmware/main_controller/include/credentials.h.example firmware/main_controller/include/credentials.h
cp firmware/esp32cam/include/credentials.h.example          firmware/esp32cam/include/credentials.h
```

Edit WiFi SSID/password, Telegram bot token, chat id.  
Camera: set static IP/gateway to match your LAN and `CAM_HOST` in Main `config.h`.

### 2. Barcode mode

In `config.h`:

```cpp
#define BARCODE_SCAN_MODE   BarcodeScanMode::Presentation
```

Optional: `Continuous`, `UartTrigger`, `HardwareTrigger`.

### 3. Seed tracking numbers

In `database.cpp`: `TH1234567890`, `TH9988776655`, `JT5566778899`  
Used flags persist in NVS. Reset: `pio run -t erase` (Main) or clear the `securedrop` namespace.

---

## Build & flash

### Prerequisites

- VS Code + [PlatformIO](https://platformio.org/)
- USB cable for Main ESP32
- FTDI / USB-TTL (3.3 V) for ESP32-CAM programming

### Flash ESP32-CAM

1. GPIO0 → GND (flash mode)  
2. Build & upload `firmware/esp32cam/`  
3. Disconnect GPIO0, RESET  
4. Serial `115200` — confirm IP (default `192.168.1.101`)  
5. Browser: `http://192.168.1.101/capture` → JPEG  

### Flash Main Controller

1. Build & upload `firmware/main_controller/`  
2. Serial `115200` — wait for FSM `WAIT_BARCODE` and `[Barcode] mode=PRESENTATION`

### Wiring checklist

- Scanner TX → ESP32 GPIO16; scanner RX → GPIO17 only if using UART_TRIGGER  
- Scanner VCC/GND per datasheet; common GND  
- Relay IN → GPIO25; relay logic supply from 5V rail; common GND  
- Relay COM → 12V+, NO → solenoid+, solenoid− → 12V GND  
- Flyback diode across solenoid  
- ESP32-CAM on same WiFi as Main  

---

## Testing procedure

1. **Boot lock** — on reset, solenoid stays locked; Serial shows `[Lock] ... default LOCKED`.  
2. **Invalid barcode** — unknown code → Telegram alert, lock never opens, NVS unused.  
3. **Valid barcode** — unlock ~5 s → capture → Telegram photo/text → lock → Serial `[Database] Marked used`.  
4. **Reuse** — same code again → `ALREADY USED`, no unlock.  
5. **Duplicate cooldown** — rapid re-scan of same label within `BARCODE_DUPLICATE_MS` → ignored.  
6. **Presentation check** — with mode `Presentation` and scanner configured for induction, lamp should be off until a code is presented. If lamp stays on, the module is still in continuous mode — use its setup barcodes (or change firmware mode after confirming trigger protocol).  
7. **Failure paths** — CAM or Telegram failure after unlock: box still relocks; tracking still marked used. WiFi down at boot: stays locked in `CONNECT_WIFI`.  

---

## Module responsibilities

| Module | Responsibility |
|---|---|
| `wifi_manager` | Connect / reconnect WiFi |
| `barcode` | Modes, states, timeout, cooldown, dedup |
| `database` | Local tracking list + NVS used flags |
| `lock` | Fail-secure relay control |
| `camera` | HTTP GET JPEG from CAM |
| `telegram` | `sendMessage` + multipart `sendPhoto` |
| `state_machine` | Delivery sequence |

---

## Known limitations

- Exact MH-ET LIVE / GM65 command set is **not** confirmed in-repo; UART/hardware trigger need your datasheet.  
- Presentation mode depends on **scanner-side** configuration; firmware cannot force the lamp off over RX-only UART without commands.  
- `PIN_BARCODE_TRIGGER` defaults to unset (`-1`).  
- CAM motion-detection arming is intentionally not implemented.  
- Tracking mark-used happens at `LOCK`; a power loss mid-unlock could leave a code reusable (box relocks on next boot).  
- Single courier camera; no door sensor / parcel presence check.  
- Serial logs omit WiFi password, Telegram token, and chat id.

---

## Folder structure

```
SecureDrop/
├── README.md
├── docs/                          # earlier full-scope design notes (superseded by v1)
└── firmware/
    ├── main_controller/
    │   ├── platformio.ini
    │   ├── include/
    │   │   ├── config.h
    │   │   ├── pins.h
    │   │   ├── barcode_commands.h
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
    └── esp32cam/
        ├── platformio.ini
        ├── include/
        │   ├── camera_pins.h
        │   └── credentials.h.example
        └── src/
            └── main.cpp
```

---

*SecureDrop v1.1 — reliable MVP first; confirm scanner model before enabling UART/hardware trigger.*
