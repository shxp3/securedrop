#include "lock.h"
#include "config.h"
#include "logger.h"

static bool lastDoorOpen = false;
static bool closeEdgeFlag = false;

static void setRelay(uint8_t pin, bool energize) {
    // รองรับ relay module ทั้งแบบ Active-HIGH และ Active-LOW ผ่าน config.h
    bool level = RELAY_ACTIVE_LOW ? !energize : energize;
    digitalWrite(pin, level ? HIGH : LOW);
}

void Lock::begin() {
    pinMode(RELAY_SOLENOID_PIN, OUTPUT);
    pinMode(DOOR_SENSOR_PIN, INPUT); // ต้องมี external pull-up (GPIO34 ไม่มี internal pull-up)
    setRelay(RELAY_SOLENOID_PIN, false); // เริ่มต้น = ล็อกอยู่ (fail-secure)
    lastDoorOpen = isDoorOpen();
    Log::info("Lock", "Solenoid lock initialized (LOCKED)");
}

void Lock::unlock() {
    setRelay(RELAY_SOLENOID_PIN, true);
    Log::info("Lock", "Solenoid UNLOCKED");
}

void Lock::lock() {
    setRelay(RELAY_SOLENOID_PIN, false);
    Log::info("Lock", "Solenoid LOCKED");
}

bool Lock::isDoorOpen() {
    // Reed switch: สมมติ HIGH = เปิด (ปรับตามการต่อจริง, ใช้ pull-up ภายนอกไป 3.3V)
    return digitalRead(DOOR_SENSOR_PIN) == HIGH;
}

bool Lock::justClosedDoor() {
    bool nowOpen = isDoorOpen();
    bool edge = (lastDoorOpen == true && nowOpen == false);
    lastDoorOpen = nowOpen;
    return edge;
}
