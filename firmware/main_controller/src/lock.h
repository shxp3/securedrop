#pragma once
// 12V Fail-Secure solenoid via 1-channel relay.

#include <Arduino.h>

class LockController {
public:
    void begin();          // defaults to LOCKED (fail-secure)
    void lock();
    void unlock();
    bool isUnlocked() const;

private:
    bool unlocked_ = false;
    void writeRelay(bool energize);
};

extern LockController lockController;
