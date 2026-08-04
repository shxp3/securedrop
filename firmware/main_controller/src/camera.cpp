#include "camera.h"
#include "config.h"
#include <HTTPClient.h>
#include <WiFi.h>

CameraClient cameraClient;

bool CameraClient::capture(CapturedImage& out) {
    out.release();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Camera] WiFi not connected");
        return false;
    }

    const String url = String("http://") + CAM_HOST + ":" + String(CAM_PORT) + CAM_CAPTURE_PATH;
    Serial.printf("[Camera] GET %s\n", url.c_str());

    HTTPClient http;
    http.setTimeout(CAM_HTTP_TIMEOUT_MS);
    if (!http.begin(url)) {
        Serial.println("[Camera] HTTP begin failed");
        return false;
    }

    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("[Camera] HTTP error: %d\n", code);
        http.end();
        return false;
    }

    const int len = http.getSize();
    WiFiClient* stream = http.getStreamPtr();
    if (stream == nullptr) {
        Serial.println("[Camera] No stream");
        http.end();
        return false;
    }

    // Prefer Content-Length; fall back to reading until closed with a cap.
    size_t capacity = (len > 0) ? static_cast<size_t>(len) : CAM_MAX_JPEG_BYTES;
    if (capacity > CAM_MAX_JPEG_BYTES) {
        Serial.printf("[Camera] Image too large (%d), abort\n", len);
        http.end();
        return false;
    }

    uint8_t* buf = static_cast<uint8_t*>(malloc(capacity));
    if (buf == nullptr) {
        Serial.println("[Camera] malloc failed");
        http.end();
        return false;
    }

    size_t received = 0;
    const unsigned long start = millis();
    while (http.connected() && (len < 0 || static_cast<int>(received) < len)) {
        if (millis() - start > CAM_HTTP_TIMEOUT_MS) {
            Serial.println("[Camera] Read timeout");
            free(buf);
            http.end();
            return false;
        }
        const size_t avail = stream->available();
        if (avail == 0) {
            delay(1);
            if (len < 0 && !stream->connected() && avail == 0) {
                break;
            }
            continue;
        }
        const size_t space = capacity - received;
        if (space == 0) {
            Serial.println("[Camera] Buffer full before end of image");
            free(buf);
            http.end();
            return false;
        }
        const size_t chunk = stream->readBytes(buf + received, min(avail, space));
        received += chunk;
    }

    http.end();

    if (received < 100) {
        Serial.printf("[Camera] Image too small (%u bytes)\n",
                      static_cast<unsigned>(received));
        free(buf);
        return false;
    }

    // Basic JPEG SOI check
    if (!(buf[0] == 0xFF && buf[1] == 0xD8)) {
        Serial.println("[Camera] Response is not JPEG");
        free(buf);
        return false;
    }

    out.data = buf;
    out.length = received;
    Serial.printf("[Camera] Captured %u bytes\n", static_cast<unsigned>(received));
    return true;
}
