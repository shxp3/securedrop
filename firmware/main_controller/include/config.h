#pragma once
// =====================================================================
//  SecureDrop - Main Controller
//  config.h : ค่าคงที่ทั้งหมดของระบบ (Pin Map, Timing, Threshold)
//  ห้ามใส่ความลับ (WiFi/Firebase/Telegram keys) ในไฟล์นี้ -> ดู secrets.h
// =====================================================================

// ---------------------- FIRMWARE INFO ----------------------
#define FW_VERSION      "1.0.0"
#define BOX_ID          "PGA-001"

// ---------------------- UART (Barcode Scanner GM65/GM66) ----------------------
#define BARCODE_RX_PIN      16   // ESP32 RX2 <- GM65 TXD
#define BARCODE_TX_PIN      17   // ESP32 TX2 -> GM65 RXD
#define BARCODE_BAUD        9600
#define BARCODE_MAX_LEN     32

// ---------------------- Lock / Relay ----------------------
#define RELAY_SOLENOID_PIN  25
#define RELAY_UV_PIN        26
#define RELAY_SIREN_PIN     27
#define RELAY_ACTIVE_LOW    true   // ปรับตามสเปกโมดูล relay ที่ใช้จริง

// ---------------------- Door / Ultrasonic ----------------------
#define DOOR_SENSOR_PIN     34     // Input only, ต้องมี external pull-up
#define ULTRASONIC_TRIG_PIN 5
#define ULTRASONIC_ECHO_PIN 18
#define ULTRASONIC_THRESHOLD_CM 15.0f   // ระยะที่ถือว่า "พบพัสดุ"
#define ULTRASONIC_SAMPLE_COUNT 5       // จำนวนตัวอย่างเฉลี่ยลด noise

// ---------------------- Motion / Vibration ----------------------
#define MPU6050_SDA_PIN     21
#define MPU6050_SCL_PIN     22
#define SW420_PIN           32
#define MOTION_ACCEL_THRESHOLD_G 1.8f
#define MOTION_GYRO_THRESHOLD_DPS 45.0f
#define VIBRATION_DEBOUNCE_MS 300
#define MOTION_CONSEC_SAMPLES 3   // ต้อง trigger ติดกันกี่ sample ก่อนถือว่าจริง (ลด false alarm)

// ---------------------- Status LEDs ----------------------
#define LED_GREEN_PIN       13
#define LED_RED_PIN         14
#define LED_YELLOW_PIN      33

// ---------------------- Camera Trigger (backup GPIO, HTTP เป็นหลัก) ----------------------
#define CAM_COURIER_TRIGGER_PIN   19
#define CAM_INTERNAL_TRIGGER_PIN  23
#define CAM_TRIGGER_PULSE_MS      100

// ---------------------- Camera IP (ตั้งแบบ Static IP บนกล้องแต่ละตัว) ----------------------
#define CAM_COURIER_IP      "192.168.1.101"
#define CAM_INTERNAL_IP     "192.168.1.102"
#define CAM_HTTP_TIMEOUT_MS 5000

// ---------------------- Timing ----------------------
#define UNLOCK_TIMEOUT_SEC      90     // เวลารอคนส่งวางพัสดุ+ปิดประตู
#define PARCEL_DETECT_TIMEOUT_SEC 90   // เวลารอ ultrasonic ตรวจพบพัสดุหลังปิดประตู
#define UV_DURATION_SEC         240    // 4 นาที (ปรับได้ 180-300)
#define FIREBASE_QUERY_TIMEOUT_MS 5000
#define HEARTBEAT_INTERVAL_MS   30000

// ---------------------- Safety ----------------------
// Hard cutoff: แม้ตั้งใจให้ 240 วิ ให้มี hard limit สูงสุดไม่เกินนี้เผื่อบั๊ก
#define UV_HARD_MAX_SEC          300

// ---------------------- OLED (optional) ----------------------
#define OLED_WIDTH   128
#define OLED_HEIGHT  64
#define OLED_ADDR    0x3C
