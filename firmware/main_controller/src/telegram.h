#pragma once
// Telegram Bot notifications (text + JPEG photo).

#include <Arduino.h>
#include "camera.h"

class TelegramNotifier {
public:
    bool begin();
    bool sendText(const String& message);
    bool sendPhoto(const CapturedImage& image, const String& caption);
    String nowTimestamp() const;

private:
    bool ensureConnected();
};

extern TelegramNotifier telegramNotifier;
