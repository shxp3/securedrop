#pragma once
// camera_config.h : Pin Map มาตรฐานของบอร์ด AI-Thinker ESP32-CAM (OV2640)
// อย่าแก้ไขขาเหล่านี้ - เป็น pin map ตายตัวตามการออกแบบวงจรของบอร์ด
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

// ขาที่เหลือใช้งานได้จริงบนบอร์ดนี้ (จำกัดมาก เพราะกล้องใช้ขาไปเกือบหมด)
#define FLASH_LED_PIN      4   // ไฟแฟลชในตัว ใช้ช่วยถ่ายภาพเวลากลางคืน
#define TRIGGER_INPUT_PIN 13   // รับสัญญาณ trigger จาก Main Controller (backup, HTTP เป็นหลัก)
#define STATUS_LED_PIN    33   // ไฟสถานะบนบอร์ด (สีแดง, active LOW)
