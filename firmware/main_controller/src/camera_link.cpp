#include "camera_link.h"
#include "config.h"
#include "logger.h"
#include <HTTPClient.h>

void CameraLink::begin() {
    pinMode(CAM_COURIER_TRIGGER_PIN, OUTPUT);
    pinMode(CAM_INTERNAL_TRIGGER_PIN, OUTPUT);
    digitalWrite(CAM_COURIER_TRIGGER_PIN, LOW);
    digitalWrite(CAM_INTERNAL_TRIGGER_PIN, LOW);
    Log::info("CameraLink", "Camera trigger pins initialized");
}

// ส่ง GPIO pulse สำรอง กรณี HTTP ล้มเหลว (กล้องตั้ง interrupt RISING เพื่อดักจับ)
static void sendGpioPulse(uint8_t pin) {
    digitalWrite(pin, HIGH);
    delay(CAM_TRIGGER_PULSE_MS);
    digitalWrite(pin, LOW);
}

static bool triggerViaHttp(const char* camIp, const String& trackingNumber) {
    HTTPClient http;
    String url = String("http://") + camIp + "/capture?tag=" + trackingNumber;
    http.setTimeout(CAM_HTTP_TIMEOUT_MS);
    http.begin(url);
    int code = http.GET();
    http.end();
    return (code == 200);
}

bool CameraLink::triggerCourierCam(const String& trackingNumber) {
    Log::infof("CameraLink", "Triggering courier cam for %s", trackingNumber.c_str());
    if (triggerViaHttp(CAM_COURIER_IP, trackingNumber)) return true;

    Log::warn("CameraLink", "HTTP trigger failed, falling back to GPIO pulse");
    sendGpioPulse(CAM_COURIER_TRIGGER_PIN);
    return true; // GPIO fallback ถือว่าส่ง trigger สำเร็จ (ผลลัพธ์จริงยืนยันผ่าน Firebase ภายหลัง)
}

bool CameraLink::triggerInternalCam(const String& trackingNumber) {
    Log::infof("CameraLink", "Triggering internal cam for %s", trackingNumber.c_str());
    if (triggerViaHttp(CAM_INTERNAL_IP, trackingNumber)) return true;

    Log::warn("CameraLink", "HTTP trigger failed, falling back to GPIO pulse");
    sendGpioPulse(CAM_INTERNAL_TRIGGER_PIN);
    return true;
}
