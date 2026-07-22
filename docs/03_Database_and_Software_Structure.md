# 3. โครงสร้างฐานข้อมูล, โครงสร้างโปรเจกต์ และ Libraries

## 3.1 การเลือกฐานข้อมูล

แนะนำ **Firebase Realtime Database + Firebase Storage** เป็นหลัก (ไม่ใช้ Google Sheets เป็นหลัก) เพราะ:

| เกณฑ์ | Firebase RTDB | Google Sheets API |
|---|---|---|
| ความหน่วง (Latency) | ต่ำ (~100-300ms) | สูงกว่า (~500ms-2s), มี rate limit เข้มงวด |
| รองรับไฟล์ภาพ | มี Firebase Storage ในตัว | ต้องพึ่ง Google Drive API แยก ซับซ้อนกว่า |
| Library บน ESP32 | เสถียร (`Firebase-ESP-Client`) | มีแต่ค่อนข้าง unofficial/community |
| Realtime Sync กับ Telegram/แอป | รองรับ Listener แบบ real-time | ไม่รองรับ ต้อง poll เอง |

> Google Sheets ยังคงมีประโยชน์เป็น **Dashboard สำหรับผู้ปกครอง/ครูดูภาพรวมข้อมูลย้อนหลัง** โดยใช้ Google Apps Script Web App เป็นตัวกลาง sync ข้อมูลจาก Firebase มาแสดงเป็นตาราง (สาธิตในงานแข่งขันได้ง่าย อ่านง่ายกว่า Firebase Console)

## 3.2 โครงสร้างฐานข้อมูล Firebase Realtime Database

```json
{
  "parcels": {
    "TH1234567890": {
      "tracking_number": "TH1234567890",
      "registered_by": "owner_uid_001",
      "registered_at": "2026-07-20T09:00:00+07:00",
      "status": "delivered",              // pending | delivered | rejected | expired
      "used": true,
      "delivered_at": "2026-07-22T14:32:00+07:00",
      "courier_photo_url": "https://firebasestorage.googleapis.com/.../courier_TH1234567890.jpg",
      "parcel_photo_url": "https://firebasestorage.googleapis.com/.../parcel_TH1234567890.jpg",
      "uv_cycle": {
        "started_at": "2026-07-22T14:33:00+07:00",
        "completed_at": "2026-07-22T14:37:00+07:00",
        "duration_sec": 240
      }
    },
    "TH9876543210": {
      "tracking_number": "TH9876543210",
      "status": "pending",
      "used": false
    }
  },

  "device_status": {
    "box_id": "PGA-001",
    "state": "IDLE_LOCKED",
    "door": "closed",
    "lock": "locked",
    "wifi_rssi": -58,
    "uptime_sec": 183940,
    "last_heartbeat": "2026-07-22T14:40:00+07:00",
    "firmware_version": "1.2.0"
  },

  "theft_events": {
    "-Nx7abc123": {
      "timestamp": "2026-07-22T02:15:33+07:00",
      "trigger_type": "vibration",        // vibration | tilt | lift | impact
      "sensor_values": { "accel_delta": 3.2, "gyro_delta": 45.1, "sw420": 1 },
      "photo_courier_url": "https://.../theft_event_1.jpg",
      "photo_internal_url": "https://.../theft_event_2.jpg",
      "acknowledged": false,
      "acknowledged_by": null,
      "acknowledged_at": null
    }
  },

  "config": {
    "uv_duration_sec": 240,
    "ultrasonic_threshold_cm": 15,
    "motion_alarm_threshold_g": 1.8,
    "vibration_debounce_ms": 300,
    "unlock_timeout_sec": 90,
    "telegram_chat_id": "123456789",
    "admin_pin_hash": "sha256:..."
  },

  "users": {
    "owner_uid_001": {
      "name": "สมชาย ใจดี",
      "telegram_chat_id": "123456789",
      "role": "admin"
    }
  }
}
```

### หลักการออกแบบ (Design Rationale)
- ใช้ `tracking_number` เป็น **Primary Key** ของ `parcels` node โดยตรง (ไม่ auto-id) เพื่อ query ตรงจุดแบบ O(1) ด้วย `GET /parcels/{tracking_number}.json` — เร็วกว่าการ query แบบกรอง (filter) ทั้งลิสต์
- แยก `device_status` ออกจาก `parcels` เพราะเป็นข้อมูลที่อัปเดตบ่อย (heartbeat ทุก 30 วิ) ถ้าอยู่ร่วมกันจะทำให้ operation อื่น ๆ ช้าลงจากขนาด payload
- `theft_events` ใช้ Firebase auto-generated key (`-Nx...`) เพราะเป็น log แบบ append-only เรียงตามเวลาธรรมชาติ
- `config` แยกออกมาให้แก้ค่าพารามิเตอร์ระบบ (เช่น ระยะเวลา UV) ได้จากระยะไกลโดยไม่ต้อง flash firmware ใหม่ — Main Controller ดึงค่านี้ตอน boot และ cache ไว้ใน RAM

### Firebase Security Rules (ตัวอย่างแนวคิด)

```json
{
  "rules": {
    "parcels": {
      "$tracking": {
        ".read": "auth != null",
        ".write": "auth != null && (auth.token.role == 'admin' || auth.token.role == 'device')"
      }
    },
    "config": {
      ".read": "auth != null",
      ".write": "auth != null && auth.token.role == 'admin'"
    },
    "theft_events": {
      ".read": "auth != null",
      ".write": "auth != null && auth.token.role == 'device'"
    }
  }
}
```

> ในระดับ competition สามารถใช้ Firebase Auth แบบ Anonymous + Custom Claims หรือใช้ Legacy Database Secret ชั่วคราวเพื่อความง่ายในการสาธิต แต่ต้องระบุในเอกสารว่า **ใน production จริงต้องเปลี่ยนเป็น Service Account + Signed Token**

## 3.3 โครงสร้างโฟลเดอร์โปรเจกต์ทั้งหมด

```
SecureDrop/
├── README.md
├── docs/
│   ├── 01_Architecture_and_Flow.md
│   ├── 02_Hardware_and_Wiring.md
│   ├── 03_Database_and_Software_Structure.md
│   ├── 04_Problems_Security_Future.md
│   ├── 05_Cost_and_Roadmap.md
│   └── 06_Testing_and_Demo_Script.md
│
└── firmware/
    ├── main_controller/                 ← PlatformIO Project (ESP32 DevKit V1)
    │   ├── platformio.ini
    │   ├── include/
    │   │   ├── config.h                 ← ค่าคงที่ pin/timing ทั้งหมด
    │   │   └── secrets.h.example        ← Template สำหรับ WiFi/Firebase/Telegram keys
    │   └── src/
    │       ├── main.cpp                 ← setup()/loop() + เรียก state machine
    │       ├── state_machine.h/.cpp     ← FSM หลักของระบบ
    │       ├── barcode.h/.cpp           ← อ่าน/ประมวลผลบาร์โค้ดจาก GM65
    │       ├── database.h/.cpp          ← ติดต่อ Firebase (CRUD parcels/config)
    │       ├── camera_link.h/.cpp       ← สั่งงาน ESP32-CAM ทั้ง 2 ตัว (HTTP+GPIO trigger)
    │       ├── notification.h/.cpp      ← ส่ง Telegram Bot message/photo
    │       ├── lock.h/.cpp              ← ควบคุม Solenoid + อ่าน Door Sensor
    │       ├── sensor.h/.cpp            ← HC-SR04 + MPU6050 + SW-420 (Sensor Fusion)
    │       ├── alarm.h/.cpp             ← ควบคุมไซเรน/บัซเซอร์ + alarm logic
    │       ├── uv.h/.cpp                ← ควบคุม UV-C + safety interlock
    │       ├── display.h/.cpp           ← (ตัวเลือก) OLED status display
    │       └── logger.h/.cpp            ← Serial + SD card logging (debug)
    │
    ├── esp32cam_courier/                ← PlatformIO Project (ESP32-CAM #1)
    │   ├── platformio.ini
    │   ├── include/secrets.h.example
    │   └── src/
    │       ├── main.cpp
    │       ├── camera_config.h          ← Pin map เฉพาะ AI-Thinker ESP32-CAM
    │       └── uploader.h/.cpp          ← อัปโหลดภาพขึ้น Firebase Storage
    │
    └── esp32cam_internal/               ← PlatformIO Project (ESP32-CAM #2)
        ├── platformio.ini
        ├── include/secrets.h.example
        └── src/
            ├── main.cpp
            ├── camera_config.h
            └── uploader.h/.cpp
```

### เหตุผลของการแบ่งโมดูล (ตรงตามที่โจทย์ระบุ + ขยายเพิ่มเพื่อความสมบูรณ์)

| โมดูล | Single Responsibility |
|---|---|
| `state_machine` | จัดการลำดับสถานะเท่านั้น ไม่ยุ่งกับ hardware โดยตรง (เรียกผ่าน interface ของโมดูลอื่น) |
| `barcode` | รับผิดชอบเฉพาะการอ่าน/ตรวจความถูกต้องรูปแบบข้อมูลจาก scanner |
| `database` | เป็นเพียง "ประตู" เดียวที่คุยกับ Firebase ทั้งหมด (Data Access Layer) โมดูลอื่นห้ามยิง HTTP ไป Firebase ตรง ๆ |
| `camera_link` | นามธรรม (abstract) การสั่งถ่ายภาพ ไม่ว่าจะผ่าน HTTP หรือ GPIO |
| `notification` | รวมศูนย์การส่งข้อความ ถ้าวันหนึ่งเปลี่ยนจาก Telegram เป็น LINE แก้แค่ไฟล์นี้ไฟล์เดียว |
| `lock` | ควบคุม physical lock + อ่านสถานะประตู เป็นเจ้าของ hardware safety logic ของประตู |
| `sensor` | รวม sensor fusion algorithm (คำนวณ threshold จาก MPU6050+SW-420) |
| `alarm` | แยกจาก sensor เพราะ alarm อาจถูกสั่งจากหลายแหล่ง (sensor, remote command, manual test) |
| `uv` | เป็นเจ้าของ **Safety Interlock Logic** ของ UV-C โดยเฉพาะ (critical safety module ต้องแยกให้ review ง่าย) |
| `logger` | เพิ่มเติมจากโจทย์ เพื่อการ debug/traceability ระดับ production |

## 3.4 Libraries ที่แนะนำ (PlatformIO / Arduino)

| Library | ใช้งานที่ไหน | เหตุผลที่เลือก |
|---|---|---|
| `Firebase-ESP-Client` (mobizt) | database.cpp, uploader.cpp | ครบเครื่องที่สุดสำหรับ ESP32, รองรับทั้ง RTDB/Firestore/Storage |
| `UniversalTelegramBot` (Witnessmenow) | notification.cpp | เสถียร รองรับ sendPhoto/sendMessage/getUpdates |
| `ArduinoJson` (bblanchon) | ทุกโมดูลที่คุย JSON | มาตรฐานอุตสาหกรรม เร็วและใช้ RAM น้อย |
| `Adafruit MPU6050` + `Adafruit Unified Sensor` | sensor.cpp | Official driver อ่าน accel/gyro ผ่าน I2C |
| `NewPing` หรือเขียนเอง | sensor.cpp (HC-SR04) | จัดการ timeout ของ echo pulse ได้ดีกว่า pulseIn ตรง ๆ |
| `ESP32Servo` (ถ้าใช้ servo แทน solenoid ในบางเวอร์ชัน) | lock.cpp | ตัวเลือกเสริม |
| `Adafruit SSD1306` + `Adafruit GFX` | display.cpp | ควบคุม OLED แสดงสถานะ |
| `esp32-camera` (Espressif official) | camera_config.h (บอร์ด CAM) | Driver กล้อง OV2640 โดยตรงจาก Espressif |
| `NTPClient` หรือ `configTime()` built-in | logger.cpp, database.cpp | Sync เวลาแม่นยำสำหรับ timestamp (RTC ภายใน ESP32 ไม่แม่นพอ) |
| `ArduinoOTA` | main.cpp | อัปเดต firmware ผ่าน WiFi โดยไม่ต้องถอดกล่องมาต่อคอม |
| `WiFiManager` (tzapu) | main.cpp | ให้ผู้ใช้ตั้งค่า WiFi ผ่าน Captive Portal แทนการ hardcode SSID/Password |

## 3.5 ตัวอย่าง `platformio.ini` (Main Controller)

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    mobizt/Firebase ESP Client @ ^4.4.17
    witnessmenow/UniversalTelegramBot @ ^1.3.0
    bblanchon/ArduinoJson @ ^6.21.5
    adafruit/Adafruit MPU6050 @ ^2.2.6
    adafruit/Adafruit Unified Sensor @ ^1.1.14
    adafruit/Adafruit SSD1306 @ ^2.5.10
    adafruit/Adafruit GFX Library @ ^1.11.9
    tzapu/WiFiManager @ ^2.0.17
build_flags =
    -D CORE_DEBUG_LEVEL=3
```
