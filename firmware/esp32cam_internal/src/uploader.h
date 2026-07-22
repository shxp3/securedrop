#pragma once
#include <Arduino.h>

namespace Uploader {
    void begin();
    String captureAndUpload(const String& tag, const char* photoFieldName);
}
