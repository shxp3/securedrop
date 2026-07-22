#pragma once
// camera_link.h : สั่งงาน ESP32-CAM ทั้งสองตัว (HTTP เป็นหลัก, GPIO pulse เป็น backup)
#include <Arduino.h>

namespace CameraLink {
    void begin();
    bool triggerCourierCam(const String& trackingNumber);   // ถ่ายรูปคนส่ง
    bool triggerInternalCam(const String& trackingNumber);   // ถ่ายรูปพัสดุในกล่อง
}
