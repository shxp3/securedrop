#pragma once
// WiFi connection helper for the Main Controller.

#include <Arduino.h>

class WifiManager {
public:
    bool begin();
    void loop();                 // non-blocking reconnect
    bool isConnected() const;
    String localIp() const;

private:
    unsigned long lastAttemptMs_ = 0;
    bool connectOnce(uint32_t timeoutMs);
};

extern WifiManager wifiManager;
