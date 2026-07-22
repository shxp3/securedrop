#include "alarm.h"
#include "config.h"
#include "logger.h"

static void setRelay(uint8_t pin, bool energize) {
    bool level = RELAY_ACTIVE_LOW ? !energize : energize;
    digitalWrite(pin, level ? HIGH : LOW);
}

void Alarm::begin() {
    pinMode(RELAY_SIREN_PIN, OUTPUT);
    setRelay(RELAY_SIREN_PIN, false);
    Log::info("Alarm", "Siren relay initialized (OFF)");
}

void Alarm::trigger() {
    setRelay(RELAY_SIREN_PIN, true);
}

void Alarm::stop() {
    setRelay(RELAY_SIREN_PIN, false);
    Log::info("Alarm", "Siren stopped (acknowledged)");
}

void Alarm::beepShort() {
    // เสียงสั้นแจ้งปฏิเสธบาร์โค้ด ไม่ใช้ relay ไซเรนหลัก (ใช้ buzzer เล็กแยกถ้ามี หรือ pulse relay สั้น ๆ)
    setRelay(RELAY_SIREN_PIN, true);
    delay(150);
    setRelay(RELAY_SIREN_PIN, false);
}
