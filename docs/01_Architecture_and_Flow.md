# 1. สถาปัตยกรรมระบบ (System Architecture) & System Flow

## 1.1 แนวคิดการออกแบบ

SecureDrop ถูกออกแบบเป็น **Distributed Embedded System** (ระบบฝังตัวแบบกระจาย) ไม่ใช่ ESP32 ตัวเดียวทำทุกอย่าง เพราะ:

- **ESP32-CAM มี RAM/Flash จำกัด** และขา GPIO ถูกใช้เกือบหมดโดยกล้อง (OV2640) → ไม่เหลือขาพอสำหรับ sensor/relay/solenoid จำนวนมาก
- **แยกหน้าที่ (Separation of Concerns)** ทำให้ debug ง่าย, โมดูลไหนพังไม่กระทบทั้งระบบ, และเพิ่มความน่าเชื่อถือ (Reliability) แบบเดียวกับสินค้าอุตสาหกรรมจริง
- ตรงกับหลักการ **Modularity > Simplicity** ตามที่ต้องการ

## 1.2 แผนภาพสถาปัตยกรรมระดับสูง (High-Level Architecture)

```
                              ┌─────────────────────────────┐
                              │        CLOUD BACKEND         │
                              │  Firebase Realtime DB /      │
                              │  Firestore + Firebase Storage│
                              └──────────────┬───────────────┘
                                             │ HTTPS / REST
                     ┌───────────────────────┼────────────────────────┐
                     │                       │                        │
             WiFi (HTTPS)              WiFi (HTTPS)             WiFi (HTTPS)
                     │                       │                        │
        ┌────────────▼───────────┐  ┌───────▼─────────┐   ┌──────────▼──────────┐
        │   MAIN CONTROLLER      │  │  ESP32-CAM #1    │   │   ESP32-CAM #2       │
        │   (ESP32 DevKit V1)    │  │  "Courier Cam"   │   │  "Internal Cam"      │
        │   - Barcode Scanner    │◄─┤  ถ่ายรูปหน้าคนส่ง │   │  ถ่ายรูปพัสดุในกล่อง │
        │   - Lock/Relay Control │  │  UART/HTTP Trigger│   │  UART/HTTP Trigger   │
        │   - Ultrasonic Sensor  │──┴──────────────────┴───┤                      │
        │   - Door Sensor        │        UART / GPIO Trigger Signal              │
        │   - MPU6050 + SW-420   │                                                │
        │   - Buzzer/Siren       │                                                │
        │   - UV-C Relay         │                                                │
        │   - Telegram Bot Client│                                                │
        │   - State Machine Core │                                                │
        └─────────────┬───────────┘                                              │
                       │ Local I/O (GPIO / I2C / UART)                            │
     ┌─────────────────┼──────────────────────────────────────────────────────┐  │
     │                 │                                                       │  │
┌────▼───┐   ┌─────────▼────────┐   ┌──────────┐   ┌─────────┐   ┌──────────┐ │  │
│ GM65/66│   │ Solenoid + Relay │   │ HC-SR04  │   │ MPU6050 │   │ SW-420   │ │  │
│Barcode │   │ + Magnetic Door  │   │Ultrasonic│   │ (I2C)   │   │Vibration │ │  │
│Scanner │   │ Sensor           │   │          │   │         │   │          │ │  │
└────────┘   └──────────────────┘   └──────────┘   └─────────┘   └──────────┘ │  │
                                                                                 │  │
             ┌──────────────────────────────────────────────────────────────┘  │
             │                                                                   │
      ┌──────▼─────┐                                                    ┌───────▼──────┐
      │ UV-C LED   │                                                    │ Buzzer/Siren │
      │ + Relay    │                                                    └──────────────┘
      └────────────┘
```

## 1.3 เหตุผลที่เลือก 3-Node Architecture

| โหนด | หน้าที่หลัก | เหตุผล |
|---|---|---|
| **Main Controller (ESP32 DevKit V1)** | ควบคุม Business Logic ทั้งหมด, State Machine, Sensor Fusion, สื่อสารกับ Cloud และ Telegram | มีขา GPIO/ADC/I2C/UART เพียงพอสำหรับอุปกรณ์จำนวนมาก, ไม่มีภาระถ่ายภาพ (ประหยัด RAM/CPU) |
| **ESP32-CAM #1 (Courier Cam)** | ถ่ายภาพใบหน้า/ท่าทางคนส่งของ ทันทีหลัง barcode ผ่าน | แยกอิสระ ป้องกันการหน่วง (latency) ของ business logic ขณะประมวลผลภาพ |
| **ESP32-CAM #2 (Internal Cam)** | ถ่ายภาพภายในกล่องเพื่อยืนยันว่ามีพัสดุจริง เป็นหลักฐาน | ตำแหน่งติดตั้งต่างจากตัวที่ 1 ต้องแยกบอร์ด |

## 1.4 System Flow แบบละเอียด (End-to-End Sequence)

```
[คนส่งของมาถึง]
      │
      ▼
[1] สแกนบาร์โค้ด (GM65/GM66) ─────────────────► Main Controller รับค่า tracking number ผ่าน UART
      │
      ▼
[2] Main Controller ส่ง HTTPS request ไปยัง Firebase
      → ตรวจสอบ: tracking number มีอยู่จริงหรือไม่ / status = "unused" หรือไม่
      │
      ├── ไม่พบ / used แล้ว ──► ปฏิเสธ, LED แดงกระพริบ, บัซเซอร์สั้น 1 ครั้ง, จบ flow
      │
      ▼ พบ และยังไม่ถูกใช้
[3] สั่ง Solenoid Lock ปลดล็อก (ผ่าน Relay) + ตั้ง state = UNLOCKED
      │
      ▼
[4] Trigger ESP32-CAM #1 ให้ถ่ายภาพคนส่ง (ผ่าน UART/GPIO pulse) → อัปโหลดภาพขึ้น Firebase Storage
      │
      ▼
[5] คนส่งเปิดประตู วางพัสดุ ปิดประตู (ตรวจสอบผ่าน Magnetic Door Sensor)
      │
      ▼
[6] HC-SR04 วัดระยะภายในกล่อง → ถ้าระยะ < threshold = ตรวจพบพัสดุ
      │
      ├── ไม่พบพัสดุภายใน timeout (เช่น 90 วิ) ──► แจ้งเตือน "ไม่พบพัสดุ" ล็อกกล่องกลับ จบ flow (ไม่ทำ UV)
      │
      ▼ พบพัสดุ + ประตูปิดสนิท (door sensor = CLOSED)
[7] ล็อกโซลินอยด์กลับอัตโนมัติ (LOCKED) + ตั้ง state = ARMED_PENDING_UV
      │
      ▼
[8] Trigger ESP32-CAM #2 ถ่ายภาพพัสดุภายในกล่อง (ภาพหลักฐาน) → อัปโหลด Firebase Storage
      │
      ▼
[9] ตรวจสอบเงื่อนไข Safety Interlock: Door=CLOSED AND Parcel=DETECTED
      → เปิด UV-C Relay
      → นับเวลา 3–5 นาที (ตั้งค่าได้)
      → ถ้า Door Sensor เปลี่ยนเป็น OPEN ระหว่างฉาย UV → ตัด UV ทันที (Hard Interrupt, ไม่รอ loop)
      │
      ▼
[10] UV ทำงานครบเวลา → ปิด UV อัตโนมัติ
      │
      ▼
[11] อัปเดต Firebase: status = "Delivered", timestamp, used = true
      │
      ▼
[12] ส่ง Telegram Notification พร้อม: Tracking Number, เวลา, สถานะ, รูปคนส่ง, รูปพัสดุ
      │
      ▼
[13] ระบบเข้าสู่ state = ARMED (โหมดเฝ้าระวังการโจรกรรม)
      │
      ▼
[ระหว่างนี้] MPU6050 + SW-420 ตรวจจับการสั่น/ยก/เอียง ตลอดเวลา (Polling 50–100 ms)
      │
      ├── ตรวจพบความผิดปกติเกิน threshold ──► [ALARM FLOW]
      │        │
      │        ▼
      │   เปิดไซเรน/บัซเซอร์ทันที (Priority Interrupt สูงสุด)
      │        │
      │        ▼
      │   Trigger กล้องทั้ง 2 ตัวถ่ายภาพเหตุการณ์
      │        │
      │        ▼
      │   ล็อกกล่อง (ถ้ายังไม่ล็อก) ป้องกันการงัดเพิ่ม
      │        │
      │        ▼
      │   ส่ง Telegram Alert: "⚠ ALERT Parcel Box Movement Detected — Possible Theft"
      │        │
      │        ▼
      │   บันทึก event log ลง Firebase (theft_events collection)
      │        │
      │        ▼
      │   รอผู้ใช้กด "Acknowledge" ผ่าน Telegram Bot Command (/ack) เพื่อปิด alarm
      │
      └── ไม่มีเหตุการณ์ผิดปกติ ──► วนลูปเฝ้าระวังต่อไปจนกว่าจะมีการสแกนบาร์โค้ดครั้งใหม่
```

## 1.5 State Machine (แผนภาพสถานะของระบบ)

ระบบใช้ **Finite State Machine (FSM)** เป็นแกนกลางของ `main.cpp` เพื่อป้องกันการทำงานผิดลำดับ (เช่น เปิด UV ทั้งที่ประตูเปิดอยู่ ซึ่งเป็นความเสี่ยงด้านความปลอดภัยที่ยอมรับไม่ได้)

```
                     ┌────────────────────────────────────────────────────┐
                     │                                                    │
                     ▼                                                    │
   ┌──────────┐  scan valid   ┌──────────┐  door closed   ┌────────────┐ │
   │  IDLE_   │──────────────►│ UNLOCKED_│───────────────►│  WAIT_     │ │
   │  LOCKED  │               │ WAITING_ │   & timeout ok │  PARCEL_   │ │
   │(ARMED)   │◄───reset──────│ COURIER  │                │  DETECT    │ │
   └────┬─────┘   /rearm      └────┬─────┘                └─────┬──────┘ │
        │                          │ timeout (no door event)     │        │
        │                          ▼                              │parcel  │
        │                    ┌──────────┐                         │detected│
        │                    │ TIMEOUT_ │                         ▼        │
        │                    │ ABORT    │                   ┌───────────┐ │
        │                    └────┬─────┘                   │ LOCKING   │ │
        │                         │                          └─────┬─────┘ │
        │                         └─────────────►IDLE_LOCKED◄──────┘       │
        │                                                                   │
        │   theft detected (any state except UV_ACTIVE-critical)            │
        │◄──────────────────────────────────────────────────────────────┐  │
        │                                                                 │  │
   ┌────▼──────┐                                                          │  │
   │  ALARM_   │────────── acknowledge via Telegram /ack ─────────────────┘  │
   │  TRIGGERED│                                                              │
   └───────────┘                                                             │
        ▲                                                                    │
        │ door opened while UV active (SAFETY VIOLATION)                     │
        │                                                                    │
   ┌────┴──────┐    UV timer complete    ┌─────────────┐  notify sent        │
   │ UV_ACTIVE │────────────────────────►│ NOTIFYING   │──────────────────────┘
   └───────────┘                          └─────────────┘
```

### รายละเอียด State ทั้งหมด

| State | ความหมาย | Guard Condition ในการเข้า | สิ่งที่ทำได้/ห้ามทำ |
|---|---|---|---|
| `IDLE_LOCKED` (ARMED) | สถานะปกติ กล่องล็อก เฝ้าระวังขโมย | เริ่มต้นระบบ หรือจบ flow ก่อนหน้า | ตรวจ MPU6050/SW-420 ตลอดเวลา, ห้ามปลดล็อกยกเว้น barcode ผ่าน |
| `SCANNING` | กำลังตรวจสอบ tracking number กับ Firebase | มีการอ่านบาร์โค้ดสำเร็จ | Timeout 5 วิ ถ้า Firebase ไม่ตอบ → กลับ IDLE_LOCKED |
| `UNLOCKED_WAITING_COURIER` | ปลดล็อกรอคนส่งเปิด-ปิดประตู | barcode valid + unused | ต้อง trigger cam#1 ทันที, ตั้ง timeout 60–90 วิ |
| `WAIT_PARCEL_DETECT` | ประตูปิดแล้ว รอตรวจสอบพัสดุด้วย HC-SR04 | door sensor = CLOSED | Timeout ถ้าไม่พบพัสดุ → ไม่เข้า UV |
| `LOCKING` | สั่งล็อกโซลินอยด์ + ตรวจสอบ feedback | parcel detected = true | ต้อง verify lock feedback (ถ้ามี sensor เสริม) |
| `UV_ACTIVE` | กำลังฉาย UV-C | door=CLOSED AND parcel=DETECTED (Hard Interlock) | **Interrupt ทันที** ถ้า door เปิดระหว่างนี้ → ตัด UV + log safety event |
| `NOTIFYING` | ส่งข้อมูลไป Telegram/Firebase | UV เสร็จสมบูรณ์ หรือ safety-abort | Retry 3 ครั้งถ้าเน็ตหลุด แล้วเก็บ queue ไว้ส่งภายหลัง |
| `ALARM_TRIGGERED` | ตรวจพบความพยายามขโมย | sensor fusion เกิน threshold ใน state ใดก็ได้ (ยกเว้นระหว่าง UV ให้ยังคง cutoff UV ก่อน) | สูงสุดของ priority, สามารถ interrupt ทุก state |
| `TIMEOUT_ABORT` | ยกเลิกเนื่องจาก timeout | ไม่มีการวางพัสดุ/ปิดประตูตามเวลา | ล็อกกล่องกลับ, แจ้งเตือนเบา ๆ, tracking กลับสถานะ pending |

## 1.6 Communication Protocol ระหว่างโมดูล

### 6.1 Main Controller ↔ Barcode Scanner (GM65/GM66)
- **Physical:** UART (TX/RX) หรือ TTL-232 mode (ไม่ใช้ USB-HID mode)
- **Protocol:** โมดูลส่งค่าบาร์โค้ดแบบ ASCII string ตามด้วย `\r\n` เมื่ออ่านสำเร็จ
- **Baud Rate:** 9600 (default ของ GM65, ตั้งค่าผ่านคำสั่ง config barcode ได้)
- Main Controller ใช้ `HardwareSerial` (Serial2) อ่านแบบ non-blocking (`available()` + buffer จนเจอ `\n`)

### 6.2 Main Controller ↔ ESP32-CAM (ทั้ง 2 ตัว)
เลือกออกแบบ 2 ทางเลือก โดยแนะนำ **แบบ HTTP REST ภายใน LAN เดียวกัน** เพราะเสถียรกว่าการส่งภาพผ่าน UART (ภาพมีขนาดใหญ่ ~ 20-50 KB ไม่เหมาะกับ UART):

```
Main Controller  ──HTTP GET http://<esp32cam-ip>/capture──►  ESP32-CAM
Main Controller  ◄──────────── JSON {status:"ok", url:"..."} ───────  ESP32-CAM (หลังอัปโหลดขึ้น Firebase Storage เอง)
```

หรือใช้สัญญาณ **GPIO Trigger Pulse** (fallback แบบง่าย ไม่ต้องพึ่ง WiFi sync กัน):
- Main Controller ส่ง pulse HIGH 100ms ไปยังขา GPIO ที่กำหนดของกล้องแต่ละตัว
- ESP32-CAM ตั้ง interrupt (RISING edge) → เมื่อ trigger จะถ่ายภาพและอัปโหลด Firebase เอง (ไม่ต้องส่งภาพผ่าน Main Controller)

**สรุป: ใช้ทั้งสองแบบร่วมกัน (Redundant Trigger)** — HTTP เป็นหลัก, GPIO pulse เป็น backup กรณี WiFi ของกล้องหลุดชั่วคราว โดยกล้องจะถ่ายและ cache ภาพไว้ใน SPIFFS/SD แล้ว retry อัปโหลดภายหลัง

### 6.3 Main Controller ↔ Firebase (Cloud)
- **Protocol:** HTTPS REST API (Firebase Realtime Database REST หรือ Firestore REST) ผ่านไลบรารี `Firebase-ESP-Client`
- **Auth:** Firebase Legacy Token หรือ Email/Password Auth (แนะนำใช้ Service Account + Custom Token สำหรับความปลอดภัยระดับ production)
- **Payload:** JSON

```json
{
  "tracking_number": "TH1234567890",
  "status": "delivered",
  "used": true,
  "timestamp": "2026-07-22T14:32:00+07:00",
  "courier_photo_url": "https://firebasestorage.../courier_TH123...jpg",
  "parcel_photo_url": "https://firebasestorage.../parcel_TH123...jpg"
}
```

### 6.4 Main Controller ↔ Telegram Bot API
- **Protocol:** HTTPS REST (Telegram Bot API `sendMessage`, `sendPhoto`)
- ไลบรารี: `UniversalTelegramBot` (Witnessmenow)
- รองรับคำสั่งย้อนกลับจากผู้ใช้ เช่น `/status`, `/ack`, `/openbox` (สำหรับเจ้าของบ้านเปิดฉุกเฉินจากระยะไกล — ต้องมี PIN ยืนยัน)

### 6.5 สรุปตารางโปรโตคอลทั้งหมด

| การเชื่อมต่อ | Physical Layer | Protocol | ทิศทาง |
|---|---|---|---|
| Main ↔ GM65/66 | UART TTL | Plain ASCII | Scanner → Main |
| Main ↔ Solenoid/Relay | GPIO Digital Out | ON/OFF | Main → Actuator |
| Main ↔ Magnetic Door Sensor | GPIO Digital In (Interrupt) | HIGH/LOW | Sensor → Main |
| Main ↔ HC-SR04 | GPIO Trigger/Echo | Pulse width timing | Main ↔ Sensor |
| Main ↔ MPU6050 | I2C (SDA/SCL) | I2C Register Read | Sensor → Main |
| Main ↔ SW-420 | GPIO Digital In (Interrupt) | HIGH/LOW pulse | Sensor → Main |
| Main ↔ Buzzer/Siren | GPIO Digital Out / PWM | ON/OFF/Tone | Main → Actuator |
| Main ↔ UV-C Relay | GPIO Digital Out | ON/OFF | Main → Actuator |
| Main ↔ ESP32-CAM x2 | WiFi (HTTP) + GPIO (backup) | REST/Pulse | Bidirectional |
| Main ↔ Firebase | WiFi (HTTPS) | REST/JSON | Bidirectional |
| Main ↔ Telegram | WiFi (HTTPS) | REST/JSON | Bidirectional |
| ESP32-CAM ↔ Firebase Storage | WiFi (HTTPS) | REST Upload | CAM → Cloud |
