#pragma once
// HTTP client that triggers ESP32-CAM /capture and stores JPEG in RAM.

#include <Arduino.h>

struct CapturedImage {
    uint8_t* data = nullptr;
    size_t length = 0;

    void release() {
        if (data != nullptr) {
            free(data);
            data = nullptr;
        }
        length = 0;
    }

    bool valid() const { return data != nullptr && length > 0; }
};

class CameraClient {
public:
    bool capture(CapturedImage& out);
};

extern CameraClient cameraClient;
