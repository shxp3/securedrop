#include "display.h"
#include "config.h"
#include "logger.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
static bool available = false;

static const char* stateName(SystemState s) {
    switch (s) {
        case SystemState::IDLE_LOCKED: return "ARMED / READY";
        case SystemState::SCANNING: return "CHECKING...";
        case SystemState::UNLOCKED_WAITING_COURIER: return "UNLOCKED";
        case SystemState::WAIT_PARCEL_DETECT: return "WAIT PARCEL";
        case SystemState::LOCKING: return "LOCKING";
        case SystemState::UV_ACTIVE: return "UV STERILIZING";
        case SystemState::NOTIFYING: return "NOTIFYING";
        case SystemState::ALARM_TRIGGERED: return "!! ALARM !!";
        case SystemState::TIMEOUT_ABORT: return "TIMEOUT";
        default: return "UNKNOWN";
    }
}

void Display::begin() {
    // ใช้ I2C bus ร่วมกับ MPU6050 (Wire.begin ถูกเรียกไปแล้วใน Sensor::begin)
    available = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
    if (!available) {
        Log::warn("Display", "OLED not found - skipping display module");
        return;
    }
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.display();
    Log::info("Display", "OLED initialized");
}

void Display::update(SystemState state, int uvSecondsRemaining) {
    if (!available) return;

    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setCursor(0, 0);
    oled.println("SecureDrop");
    oled.println("--------------------");
    oled.setTextSize(1);
    oled.setCursor(0, 20);
    oled.println(stateName(state));

    if (state == SystemState::UV_ACTIVE) {
        oled.setCursor(0, 40);
        oled.printf("UV remaining: %ds", uvSecondsRemaining);
    }
    oled.display();
}
