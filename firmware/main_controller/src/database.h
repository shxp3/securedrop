#pragma once
// database.h : Data Access Layer เดียวที่คุยกับ Firebase ทั้งหมด
// โมดูลอื่น "ห้าม" เรียก Firebase library ตรง ๆ ต้องผ่านชั้นนี้เท่านั้น (encapsulation)
#include <Arduino.h>

namespace Database {
    struct ParcelRecord {
        String trackingNumber;
        String status;      // pending | delivered | rejected | expired
        bool used = false;
    };

    struct SystemConfig {
        int uvDurationSec = 240;
        float ultrasonicThresholdCm = 15.0f;
        float motionAccelThresholdG = 1.8f;
        int unlockTimeoutSec = 90;
    };

    void begin();
    void loop();  // เรียก Firebase.ready() housekeeping ทุก loop()

    bool lookupParcel(const String& trackingNumber, ParcelRecord& outRecord);
    bool updateParcelStatus(const String& trackingNumber, const String& status, bool used);
    bool attachPhotoUrl(const String& trackingNumber, const char* field, const String& url);

    bool logTheftEvent(const String& triggerType, float accelDelta, float gyroDelta);
    bool sendHeartbeat(const String& state);

    SystemConfig fetchConfig(); // ดึง config จาก /config node (cache ไว้ที่ boot)
}
