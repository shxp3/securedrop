#include "lock.h"
#include "pins.h"

LockController lockController;

void LockController::writeRelay(bool energize) {
    // Fail-Secure lock: coil energized = unlocked, de-energized = locked.
    const int level = RELAY_ACTIVE_LOW ? (energize ? LOW : HIGH)
                                       : (energize ? HIGH : LOW);
    digitalWrite(PIN_RELAY, level);
}

void LockController::begin() {
    pinMode(PIN_RELAY, OUTPUT);
    unlocked_ = false;
    writeRelay(false);  // locked at boot
    Serial.printf("[Lock] Relay GPIO%d active_%s — default LOCKED\n",
                  PIN_RELAY, RELAY_ACTIVE_LOW ? "low" : "high");
}

void LockController::lock() {
    writeRelay(false);
    unlocked_ = false;
    Serial.println("[Lock] LOCKED");
}

void LockController::unlock() {
    writeRelay(true);
    unlocked_ = true;
    Serial.println("[Lock] UNLOCKED");
}

bool LockController::isUnlocked() const {
    return unlocked_;
}
