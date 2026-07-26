#include "uploader.h"
#include "esp_camera.h"
#include <Firebase.h>
#include "secrets.h"

static FirebaseData fbdo;
static FirebaseAuth auth;
static FirebaseConfig fbConfig;

void Uploader::begin() {
    fbConfig.host = FIREBASE_HOST;
    fbConfig.api_key = FIREBASE_API_KEY;
    auth.user.email = FIREBASE_AUTH_EMAIL;
    auth.user.password = FIREBASE_AUTH_PASS;
    Firebase.begin(&fbConfig, &auth);
    Firebase.reconnectWiFi(true);
}

String Uploader::captureAndUpload(const String& tag, const char* photoFieldName) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("[Uploader] Camera capture failed");
        return "";
    }

    String remotePath = "/parcel_photos/" + tag + "_" + String(millis()) + ".jpg";

    // Firebase-ESP-Client Storage upload API (simplified conceptual usage):
    bool ok = Firebase.Storage.upload(&fbdo, FIREBASE_STORAGE_BUCKET, fb->buf, fb->len,
                                       remotePath.c_str(), "image/jpeg");

    esp_camera_fb_return(fb);

    if (!ok) {
        Serial.printf("[Uploader] Upload failed: %s\n", fbdo.errorReason().c_str());
        return ""; // TODO: cache to SPIFFS/SD และ retry ภายหลังถ้าต้องการความทนทานสูงขึ้น
    }

    String downloadUrl = fbdo.downloadURL();

    // บันทึก URL กลับเข้า Realtime Database เพื่อให้ Main Controller/Telegram ใช้งานต่อได้
    String dbPath = "/parcels/" + tag + "/" + String(photoFieldName);
    Firebase.RTDB.setString(&fbdo, dbPath, downloadUrl);

    Serial.printf("[Uploader] Uploaded: %s\n", downloadUrl.c_str());
    return downloadUrl;
}
