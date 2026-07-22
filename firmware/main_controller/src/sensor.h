#pragma once
// sensor.h : HC-SR04 (parcel detection) + MPU6050/SW-420 (theft/motion sensor fusion)
#include <Arduino.h>

namespace Sensor {
    void begin();
    void update();               // เรียกทุก loop() เพื่ออ่านค่าเซนเซอร์ต่อเนื่อง

    bool isParcelDetected();     // จาก HC-SR04 (เฉลี่ยหลาย sample)
    bool isTheftDetected();      // sensor fusion ของ MPU6050 + SW-420 พร้อม debounce/hysteresis

    // ค่าดิบสำหรับ logging / theft event
    float lastAccelDeltaG();
    float lastGyroDeltaDps();
}
