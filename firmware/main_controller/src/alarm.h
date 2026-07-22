#pragma once
// alarm.h : ควบคุมไซเรน/บัซเซอร์ - แยกจาก sensor เพราะอาจถูกสั่งจากหลายแหล่ง
#include <Arduino.h>

namespace Alarm {
    void begin();
    void trigger();     // เปิดไซเรนต่อเนื่อง (จนกว่าจะ acknowledge)
    void stop();
    void beepShort();   // เสียงสั้น ๆ สำหรับแจ้งปฏิเสธบาร์โค้ด (ไม่ใช่ alarm เต็มรูปแบบ)
}
