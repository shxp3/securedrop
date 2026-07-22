#pragma once
// barcode.h : รับผิดชอบเฉพาะการอ่าน/ตรวจรูปแบบข้อมูลจาก GM65/GM66 (UART TTL mode)
#include <Arduino.h>

namespace Barcode {
    void begin();
    void poll();                 // เรียกทุก loop() เพื่ออ่าน UART buffer แบบ non-blocking
    bool hasNewScan();           // มีบาร์โค้ดใหม่รออ่านหรือไม่
    String readTracking();       // ดึงค่าล่าสุด (ล้าง flag hasNewScan)
    bool isValidFormat(const String& code); // ตรวจรูปแบบเบื้องต้น (ความยาว/ตัวอักษรที่อนุญาต)
}
