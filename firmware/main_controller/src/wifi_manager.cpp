#include "wifi_manager.h"
#include "config.h"
#include "credentials.h"
#include <WiFi.h>

WifiManager wifiManager;

bool WifiManager::connectOnce(uint32_t timeoutMs) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("[WiFi] Connecting to '%s'...\n", WIFI_SSID);

    const unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
        delay(250);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFi] Connected, IP=%s RSSI=%d\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        return true;
    }

    Serial.println("[WiFi] Connect failed");
    return false;
}

bool WifiManager::begin() {
    lastAttemptMs_ = millis();
    return connectOnce(WIFI_CONNECT_TIMEOUT_MS);
}

void WifiManager::loop() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }
    if (millis() - lastAttemptMs_ < WIFI_RECONNECT_INTERVAL_MS) {
        return;
    }
    lastAttemptMs_ = millis();
    Serial.println("[WiFi] Reconnecting...");
    WiFi.disconnect(true);
    delay(100);
    connectOnce(WIFI_CONNECT_TIMEOUT_MS);
}

bool WifiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String WifiManager::localIp() const {
    return WiFi.localIP().toString();
}
