#include "uv.h"
#include "config.h"
#include "logger.h"

static bool active = false;
static unsigned long startedAt = 0;
static int plannedDurationSec = 0;

static void setRelay(uint8_t pin, bool energize) {
    bool level = RELAY_ACTIVE_LOW ? !energize : energize;
    digitalWrite(pin, level ? HIGH : LOW);
}

void Uv::begin() {
    pinMode(RELAY_UV_PIN, OUTPUT);
    setRelay(RELAY_UV_PIN, false); // ปิดเสมอเมื่อ boot (fail-safe default)
    active = false;
    Log::info("UV", "UV-C relay initialized (OFF)");
}

void Uv::start(int durationSec) {
    // Hard cap: ไม่ว่าจะสั่งกี่วิ ห้ามเกิน UV_HARD_MAX_SEC เด็ดขาด
    plannedDurationSec = min(durationSec, UV_HARD_MAX_SEC);
    startedAt = millis();
    active = true;
    setRelay(RELAY_UV_PIN, true);
    Log::infof("UV", "UV-C cycle started for %d seconds", plannedDurationSec);
}

void Uv::forceOff(const char* reason) {
    if (active) {
        setRelay(RELAY_UV_PIN, false);
        active = false;
        Log::errorf("UV", "UV-C FORCE OFF - reason: %s", reason);
    }
}

bool Uv::isActive() { return active; }

bool Uv::isComplete() {
    if (!active) return false;

    unsigned long elapsedSec = (millis() - startedAt) / 1000UL;

    // Hard safety cutoff อิสระจาก planned duration เผื่อ millis() overflow/บั๊ก
    if (elapsedSec >= (unsigned long)plannedDurationSec || elapsedSec >= UV_HARD_MAX_SEC) {
        setRelay(RELAY_UV_PIN, false);
        active = false;
        Log::info("UV", "UV-C cycle completed normally");
        return true;
    }
    return false;
}

int Uv::secondsRemaining() {
    if (!active) return 0;
    long elapsed = (millis() - startedAt) / 1000UL;
    return max(0L, (long)plannedDurationSec - elapsed);
}
