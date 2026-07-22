# 2. ฮาร์ดแวร์, Pin Assignment และ Wiring Diagram

## 2.1 รายการฮาร์ดแวร์ทั้งหมด (Bill of Materials Overview)

| หมวด | อุปกรณ์ | จำนวน | หมายเหตุ |
|---|---|---|---|
| ควบคุมหลัก | ESP32 DevKit V1 (38 pin) | 1 | สมองหลักของระบบ |
| กล้อง | ESP32-CAM (OV2640) | 2 | ต้องมี FTDI/USB-TTL แยกสำหรับ flash โปรแกรม |
| บาร์โค้ด | GM65 / GM66 Barcode Scanner Module | 1 | โหมด UART TTL |
| ล็อก | Solenoid Lock 12V (Fail-Secure แนะนำ) | 1 | ล็อกเมื่อไม่มีไฟ ปลอดภัยกว่า |
| รีเลย์ | Relay Module 5V 1-channel (แยก 3 ตัว) | 3 | Solenoid / UV-C / Siren (แนะนำแยกช่องสัญญาณ) |
| ประตู | Magnetic Door Sensor (Reed Switch) | 1 | |
| ระยะ | HC-SR04 Ultrasonic Sensor | 1 | ตรวจพัสดุในกล่อง |
| การเคลื่อนไหว | MPU6050 (Accel+Gyro, I2C) | 1 | ตรวจยก/เอียง |
| สั่นสะเทือน | SW-420 Vibration Sensor | 1 | ตรวจการงัด/กระแทก |
| แจ้งเตือนเสียง | Buzzer Active 5V / ไซเรน 12V | 1 | ผ่าน relay ถ้าเป็นไซเรน 12V |
| ฆ่าเชื้อ | UV-C LED Module (275nm) | 1-2 | ต้องอยู่ในตำแหน่งปิดมิดชิด ไม่รั่วออกนอกกล่อง |
| แหล่งจ่ายไฟ | Adapter 12V 3A | 1 | จ่าย Solenoid + UV + Siren |
| แปลงไฟ | Buck Converter 12V→5V (LM2596) | 1-2 | จ่าย ESP32 และโมดูลลอจิก |
| แสดงผล (ตัวเลือก) | OLED 0.96" I2C (SSD1306) | 1 | แสดงสถานะระบบหน้ากล่อง |
| ไฟสถานะ (ตัวเลือก) | LED สีเขียว/แดง/เหลือง | 3 | บอกสถานะ ready/error/processing |
| บันทึกข้อมูล (ตัวเลือก) | MicroSD Card Module | 1 | เก็บ log/รูปสำรองกรณีเน็ตหลุด |
| น้ำหนัก (ตัวเลือก) | Load Cell 5kg + HX711 | 1 | ยืนยันน้ำหนักพัสดุเพิ่มความแม่นยำ |

## 2.2 ผังพลังงาน (Power Architecture)

```
[Adapter 220VAC → 12VDC 3A]
        │
        ├──────────────► Solenoid Lock (ผ่าน Relay #1)   ~1-2A ขณะทำงาน (peak)
        │
        ├──────────────► UV-C LED Module (ผ่าน Relay #2)  ~0.3-0.5A
        │
        ├──────────────► Siren/Buzzer 12V (ผ่าน Relay #3) ~0.2A
        │
        └──────────────► Buck Converter (12V → 5V 3A)
                                │
                                ├──► ESP32 DevKit V1 (Main)     ~0.3-0.5A (WiFi TX peak ~0.8A)
                                ├──► ESP32-CAM #1                ~0.3-0.5A (peak ~0.8-1A ขณะถ่ายภาพ)
                                ├──► ESP32-CAM #2                ~0.3-0.5A
                                ├──► GM65 Barcode Scanner        ~0.1A
                                ├──► HC-SR04, MPU6050, SW-420    ~0.05A รวม
                                └──► OLED / LED สถานะ            ~0.05A
```

**ข้อควรระวังสำคัญ:**
- ESP32-CAM กิน current peak สูงมากช่วง WiFi TX + ถ่ายภาพ (อาจถึง 1A) → **ต้องใช้ Buck Converter แยกจาก ESP32 หลัก หรืออย่างน้อยต้องมี capacitor 470-1000 µF คร่อมขา 5V/GND ของแต่ละบอร์ด** ไม่เช่นนั้นบอร์ดจะ brown-out รีสตาร์ทเอง (ปัญหาคลาสสิกของ ESP32-CAM)
- ห้ามจ่ายไฟ Solenoid/UV/Siren ผ่านขา 5V ของ ESP32 โดยตรง ต้องผ่าน Relay ที่แยก power domain (Solenoid ใช้ 12V ต่างหาก)
- ใส่ **ฟิวส์ (Fuse) 3A** ที่ทางเข้า 12V และ **ไดโอด Flyback** คร่อม Solenoid Lock เพื่อป้องกันแรงดันย้อนกลับ (back-EMF) ทำลาย Relay/MCU

## 2.3 Pin Assignment — ESP32 DevKit V1 (Main Controller)

> ESP32 DevKit V1 มี GPIO ที่ใช้งานได้จริงประมาณ 25 ขา ต้องหลีกเลี่ยงขา Strapping (GPIO 0, 2, 12, 15) และขาที่ใช้ Flash ภายใน (GPIO 6-11)

| Function | GPIO | ประเภท | หมายเหตุ |
|---|---|---|---|
| Barcode RX (จาก GM65 TX) | GPIO 16 (RX2) | UART2 RX | HardwareSerial(2) |
| Barcode TX (ไป GM65 RX) | GPIO 17 (TX2) | UART2 TX | ใช้เฉพาะถ้าต้องส่ง config command |
| Solenoid Lock Relay | GPIO 25 | Digital Out | Active LOW module ให้ระวัง logic |
| UV-C Relay | GPIO 26 | Digital Out | |
| Siren/Buzzer Relay | GPIO 27 | Digital Out | |
| Magnetic Door Sensor | GPIO 34 (Input Only) | Digital In + Interrupt | ต่อ Pull-up ภายนอก (ขา 34-39 ไม่มี internal pull-up) |
| HC-SR04 Trigger | GPIO 5 | Digital Out | |
| HC-SR04 Echo | GPIO 18 | Digital In | ผ่าน Voltage Divider (5V→3.3V) |
| MPU6050 SDA | GPIO 21 | I2C SDA | |
| MPU6050 SCL | GPIO 22 | I2C SCL | |
| MPU6050 INT (ตัวเลือก) | GPIO 35 (Input Only) | Interrupt | Motion interrupt แทน polling |
| SW-420 Vibration | GPIO 32 | Digital In + Interrupt | |
| OLED SDA (ตัวเลือก) | GPIO 21 | I2C SDA (share bus กับ MPU6050) | ใช้ address แยกกัน |
| OLED SCL (ตัวเลือก) | GPIO 22 | I2C SCL (share bus) | |
| Status LED เขียว | GPIO 13 | Digital Out | Ready/Armed |
| Status LED แดง | GPIO 14 | Digital Out | Error/Alarm |
| Status LED เหลือง | GPIO 33 | Digital Out | Processing |
| Trigger CAM#1 (Courier) | GPIO 19 | Digital Out (Pulse) | Backup trigger ถ้า HTTP ล้มเหลว |
| Trigger CAM#2 (Internal) | GPIO 23 | Digital Out (Pulse) | Backup trigger |
| MicroSD (ตัวเลือก) CS | GPIO 4 | SPI CS | ถ้าใช้ SPI SD card module แยก |
| MicroSD SCK/MISO/MOSI | GPIO 18/19/23 (ใช้ VSPI default) | SPI | **ระวังชนกับขา trigger cam ด้านบน หากใช้ SD ให้ย้าย trigger ไปขาอื่น เช่น GPIO 15(ระวัง strap)/GPIO 2** |
| Load Cell HX711 DT (ตัวเลือก) | GPIO 15 | Digital In | ระวัง strap pin ให้ปล่อยลอยตอน boot |
| Load Cell HX711 SCK (ตัวเลือก) | GPIO 2 | Digital Out | ระวัง strap pin |

> **หมายเหตุสำคัญ:** GPIO 34-39 เป็น **Input-only** ใช้ต่อ sensor เท่านั้น ต่อ actuator ไม่ได้ และไม่มี internal pull-up/pull-down ต้องต่อ R ภายนอกเอง (10kΩ)

## 2.4 Pin Assignment — ESP32-CAM (AI-Thinker Module)

ESP32-CAM ขา GPIO ถูกใช้เกือบหมดโดยกล้อง OV2640 เหลือใช้งานได้จริงไม่กี่ขา:

| Function | GPIO | หมายเหตุ |
|---|---|---|
| Camera (OV2640) | GPIO 0,5,18,19,21,22,23,25,26,27,32,34,35,36,39 | ใช้ภายในโดย library `esp32-camera` ห้ามนำไปใช้งานอื่น |
| Flash LED (built-in) | GPIO 4 | ใช้เป็นไฟช่วยถ่ายภาพตอนกลางคืน |
| Trigger Input (จาก Main Controller) | GPIO 13 | ขาที่เหลือใช้ได้จริงมีน้อยมาก, ตั้งเป็น interrupt RISING |
| Status LED (บนบอร์ด, สีแดง) | GPIO 33 | ใช้บอกสถานะถ่ายภาพสำเร็จ |
| MicroSD (built-in slot, ตัวเลือก) | GPIO 2,4,12,13,14,15 (SDMMC) | ใช้เก็บภาพสำรอง แต่จะชนกับ GPIO 4 (flash) และ GPIO 13 (trigger) ต้องออกแบบร่วมกันดี ๆ |

> **ข้อจำกัดสำคัญของ ESP32-CAM:** มีขาว่างจริงใช้ได้สะดวกเพียง GPIO 12, 13, 14, 15, 16 (และบางขาชนกับ SD card ถ้าจะใช้) → **แนะนำใช้การสื่อสารผ่าน WiFi/HTTP เป็นหลัก ลด reliance บน GPIO ให้เหลือเฉพาะ trigger สำรอง**

## 2.5 Wiring Diagram แบบข้อความ (ASCII Wiring Reference)

```
┌───────────────────────────── ESP32 DevKit V1 (MAIN) ─────────────────────────────┐
│                                                                                     │
│  GPIO16 (RX2) ───────────────────────────────────► GM65 TXD                       │
│  GPIO17 (TX2) ───────────────────────────────────► GM65 RXD                       │
│  5V / GND ────────────────────────────────────────► GM65 VCC/GND                  │
│                                                                                     │
│  GPIO25 ──────► [Relay#1 IN] ──► Relay#1 COM/NO ──► Solenoid Lock (+) ──► 12V      │
│                                    Solenoid Lock (-) ──► GND (12V rail)            │
│                                    (มี Flyback Diode ขนานกับขดลวด Solenoid)         │
│                                                                                     │
│  GPIO26 ──────► [Relay#2 IN] ──► Relay#2 COM/NO ──► UV-C Module (+) ──► 12V/5V     │
│  GPIO27 ──────► [Relay#3 IN] ──► Relay#3 COM/NO ──► Siren (+) ──► 12V              │
│                                                                                     │
│  GPIO34 ◄───── Magnetic Door Sensor (ต่อร่วมกับ R pull-up 10kΩ ไป 3.3V)             │
│                                                                                     │
│  GPIO5  ─────► HC-SR04 TRIG                                                        │
│  GPIO18 ◄───── HC-SR04 ECHO (ผ่าน Voltage Divider R1=1kΩ, R2=2kΩ ลดจาก 5V→3.3V)     │
│  5V/GND ─────► HC-SR04 VCC/GND                                                     │
│                                                                                     │
│  GPIO21 (SDA) ◄──► MPU6050 SDA   (Pull-up 4.7kΩ ทั้ง SDA,SCL ไป 3.3V ถ้าโมดูลไม่มี)  │
│  GPIO22 (SCL) ◄──► MPU6050 SCL                                                     │
│  3.3V/GND ────► MPU6050 VCC/GND                                                    │
│                                                                                     │
│  GPIO32 ◄───── SW-420 DO (Digital Out) + Pull-down 10kΩ                            │
│                                                                                     │
│  GPIO13/14/33 ──► LED เขียว/แดง/เหลือง (ผ่าน R 220-330Ω ตัวละดวง)                    │
│                                                                                     │
│  GPIO19 ──────► Trigger Line ไปยัง ESP32-CAM #1 (GPIO13 ของกล้อง)                   │
│  GPIO23 ──────► Trigger Line ไปยัง ESP32-CAM #2 (GPIO13 ของกล้อง)                   │
│                  (Common GND ต้องเชื่อมทุกบอร์ดเข้าด้วยกัน)                          │
└─────────────────────────────────────────────────────────────────────────────────┘
```

## 2.6 ข้อควรระวังด้านความปลอดภัยไฟฟ้า (Electrical Safety)

1. **แยก Ground Plane** ระหว่างวงจร 12V (Solenoid/UV/Siren) กับวงจรลอจิก 3.3V/5V ให้เชื่อมกันที่จุดเดียว (Single Point Ground) เพื่อลด noise
2. **Optoisolation:** แนะนำใช้ Relay Module ที่มี Opto-isolator ในตัว (มีขายทั่วไปราคาถูก) เพื่อป้องกันไฟกระชากย้อนเข้า ESP32
3. **UV-C Safety:** UV-C 275nm เป็นอันตรายต่อดวงตา/ผิวหนัง **ต้องมี Hardware Interlock แบบ Fail-Safe** ไม่ใช่พึ่งซอฟต์แวร์อย่างเดียว — แนะนำต่อ Magnetic Door Sensor แบบอนุกรม (in-series) กับวงจรไฟเลี้ยงของ UV Relay physically เพื่อว่าแม้ซอฟต์แวร์ค้าง/บั๊ก UV ก็ไม่สามารถติดได้เมื่อประตูเปิด (Defense in Depth)
4. **Enclosure:** UV-C Module ต้องติดตั้งในช่องแยกที่ไม่มีแสงรั่วออกสู่ภายนอกหรือช่องที่คนสัมผัสได้
5. **Fuse/PTC** ที่ทุกสาย 12V เพื่อป้องกันไฟฟ้าลัดวงจร

## 2.7 ข้อเสนอแนะปรับปรุงฮาร์ดแวร์ (Recommended Hardware Improvements)

| ข้อเสนอ | เหตุผล |
|---|---|
| เปลี่ยนจาก Active Buzzer เป็น **ไซเรนแบบมีแบตเตอรี่สำรองในตัว** | ถ้าขโมยตัดสายไฟหลัก ไซเรนยังทำงานได้ |
| เพิ่ม **UPS/Li-ion Backup (18650 + BMS + Boost)** สำหรับ ESP32 หลัก | กันไฟดับแล้วระบบเฝ้าระวังหยุดทำงาน |
| ใช้ **Solenoid Lock แบบ Fail-Secure** ไม่ใช่ Fail-Safe | ไฟดับแล้วต้องยังคงล็อกอยู่ (ป้องกันขโมยตัดไฟเพื่อปลดล็อก) |
| เพิ่ม **RFID/NFC Tag สำรอง** สำหรับเจ้าของเปิดฉุกเฉิน | เผื่อกรณี WiFi/Firebase ล่ม ยังเปิดกล่องได้ |
| เพิ่ม Load Cell + HX711 | ป้องกันการ "แกล้งสแกนบาร์โค้ดปลอมแล้วไม่วางพัสดุจริง" โดยยืนยันน้ำหนักคู่กับ ultrasonic |
| ใช้ตัวถังกล่องเป็นโลหะ + เคลือบกันสนิม | ทนต่อการงัดแงะทางกายภาพจริง (เพิ่มความน่าเชื่อถือระดับสินค้าเชิงพาณิชย์) |
| เพิ่ม Watchdog Timer ภายนอก (เช่น IC TPL5010) | รีเซ็ต ESP32 อัตโนมัติถ้าโปรแกรมค้าง (Hardware Watchdog เสริมจาก Software Watchdog) |
