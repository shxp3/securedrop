#pragma once
// lock.h : ควบคุม Solenoid Lock + อ่าน Magnetic Door Sensor
#include <Arduino.h>

namespace Lock {
    void begin();
    void unlock();
    void lock();
    bool isDoorOpen();
    bool justClosedDoor();   // edge-detect: true เพียงครั้งเดียวตอนประตูเปลี่ยนจากเปิด->ปิด
}
