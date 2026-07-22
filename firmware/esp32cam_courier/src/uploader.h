#pragma once
// uploader.h : อัปโหลดภาพขึ้น Firebase Storage แล้วบันทึก URL กลับเข้า Realtime Database
#include <Arduino.h>

namespace Uploader {
    void begin();
    // ถ่ายภาพและอัปโหลด คืนค่า download URL หรือ "" ถ้าล้มเหลว
    String captureAndUpload(const String& tag, const char* photoFieldName);
}
