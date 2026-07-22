#include "database.h"
#include "config.h"
#include "logger.h"
#include <Firebase_ESP_Client.h>
#include "secrets.h"

static FirebaseData fbdo;
static FirebaseAuth auth;
static FirebaseConfig fbConfig;

void Database::begin() {
    fbConfig.host = FIREBASE_HOST;
    fbConfig.api_key = FIREBASE_API_KEY;
    auth.user.email = FIREBASE_AUTH_EMAIL;
    auth.user.password = FIREBASE_AUTH_PASS;

    Firebase.begin(&fbConfig, &auth);
    Firebase.reconnectWiFi(true);
    fbdo.setBSSLBufferSize(4096, 1024); // ลดใช้ RAM สำหรับ payload ขนาดเล็ก

    Log::info("Database", "Firebase initialized");
}

void Database::loop() {
    // Firebase-ESP-Client จัดการ token refresh อัตโนมัติภายใน library
    // ฟังก์ชันนี้เผื่อไว้สำหรับ housekeeping เพิ่มเติมในอนาคต (เช่น retry queue)
}

bool Database::lookupParcel(const String& trackingNumber, ParcelRecord& outRecord) {
    String path = "/parcels/" + trackingNumber;
    if (!Firebase.RTDB.getJSON(&fbdo, path)) {
        Log::errorf("Database", "lookupParcel failed: %s", fbdo.errorReason().c_str());
        return false;
    }
    if (fbdo.dataType() == "null") {
        return false; // ไม่พบ tracking number ในระบบ
    }

    FirebaseJson* json = fbdo.jsonObjectPtr();
    FirebaseJsonData result;

    outRecord.trackingNumber = trackingNumber;

    json->get(result, "status");
    outRecord.status = result.success ? result.stringValue.c_str() : "unknown";

    json->get(result, "used");
    outRecord.used = result.success ? result.boolValue : false;

    return true;
}

bool Database::updateParcelStatus(const String& trackingNumber, const String& status, bool used) {
    FirebaseJson json;
    json.set("status", status);
    json.set("used", used);
    json.set("delivered_at", (int)time(nullptr));

    String path = "/parcels/" + trackingNumber;
    bool ok = Firebase.RTDB.updateNode(&fbdo, path, &json);
    if (!ok) Log::errorf("Database", "updateParcelStatus failed: %s", fbdo.errorReason().c_str());
    return ok;
}

bool Database::attachPhotoUrl(const String& trackingNumber, const char* field, const String& url) {
    String path = "/parcels/" + trackingNumber + "/" + String(field);
    bool ok = Firebase.RTDB.setString(&fbdo, path, url);
    if (!ok) Log::errorf("Database", "attachPhotoUrl failed: %s", fbdo.errorReason().c_str());
    return ok;
}

bool Database::logTheftEvent(const String& triggerType, float accelDelta, float gyroDelta) {
    FirebaseJson json;
    json.set("timestamp", (int)time(nullptr));
    json.set("trigger_type", triggerType);
    json.set("sensor_values/accel_delta", accelDelta);
    json.set("sensor_values/gyro_delta", gyroDelta);
    json.set("acknowledged", false);

    bool ok = Firebase.RTDB.pushJSON(&fbdo, "/theft_events", &json);
    if (!ok) Log::errorf("Database", "logTheftEvent failed: %s", fbdo.errorReason().c_str());
    return ok;
}

bool Database::sendHeartbeat(const String& state) {
    FirebaseJson json;
    json.set("box_id", BOX_ID);
    json.set("state", state);
    json.set("uptime_sec", (int)(millis() / 1000));
    json.set("firmware_version", FW_VERSION);
    json.set("last_heartbeat", (int)time(nullptr));

    return Firebase.RTDB.updateNode(&fbdo, "/device_status", &json);
}

Database::SystemConfig Database::fetchConfig() {
    SystemConfig cfg; // ค่า default ตาม config.h ก่อน แล้ว override ถ้ามีค่าใน Firebase

    if (Firebase.RTDB.getJSON(&fbdo, "/config")) {
        FirebaseJson* json = fbdo.jsonObjectPtr();
        FirebaseJsonData result;

        if (json->get(result, "uv_duration_sec")) cfg.uvDurationSec = result.intValue;
        if (json->get(result, "ultrasonic_threshold_cm")) cfg.ultrasonicThresholdCm = result.floatValue;
        if (json->get(result, "motion_alarm_threshold_g")) cfg.motionAccelThresholdG = result.floatValue;
        if (json->get(result, "unlock_timeout_sec")) cfg.unlockTimeoutSec = result.intValue;

        Log::info("Database", "Remote config loaded from Firebase");
    } else {
        Log::warn("Database", "Using default local config (Firebase config fetch failed)");
    }
    return cfg;
}
