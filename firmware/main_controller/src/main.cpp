// =====================================================================
//  SecureDrop - Main Controller (ESP32 DevKit V1)
//  main.cpp : จุดเริ่มต้นโปรแกรม ทำหน้าที่เพียง init โมดูลและเรียก
//             state machine ทุก loop() - business logic ทั้งหมดอยู่ในโมดูลย่อย
// =====================================================================
#include <Arduino.h>
#include <WiFi.h>
#include "secrets.h"
#include "config.h"
#include "logger.h"
#include "barcode.h"
#include "database.h"
#include "camera_link.h"
#include "notification.h"
#include "lock.h"
#include "sensor.h"
#include "alarm.h"
#include "uv.h"
#include "display.h"
#include "state_machine.h"

static unsigned long lastHeartbeat = 0;

static void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Log::infof("Main", "Connecting to WiFi: %s", WIFI_SSID);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
        delay(300);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Log::infof("Main", "WiFi connected, IP: %s", WiFi.localIP().toString().c_str());
    } else {
        Log::error("Main", "WiFi connection failed - will retry in background via reconnectWiFi()");
    }
}

void setup() {
    Log::begin();
    Log::infof("Main", "SecureDrop v%s booting...", FW_VERSION);

    connectWiFi();

    // Sync เวลาแม่นยำผ่าน NTP ก่อนเริ่มงานจริง (สำคัญสำหรับ timestamp)
    configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // UTC+7 (Thailand)

    // Init LEDs
    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_YELLOW_PIN, OUTPUT);

    // Init hardware modules (ลำดับสำคัญ: safety-critical ก่อน)
    Lock::begin();          // ต้อง default เป็น LOCKED เสมอ (fail-secure)
    Uv::begin();            // ต้อง default เป็น OFF เสมอ
    Alarm::begin();
    Sensor::begin();
    CameraLink::begin();
    Barcode::begin();
    Database::begin();
    Notification::begin();
    Display::begin();

    stateMachine.begin();

    digitalWrite(LED_GREEN_PIN, HIGH); // ระบบพร้อมทำงาน
    Log::info("Main", "System ready - ARMED");
}

void loop() {
    // --- Input polling (non-blocking) ---
    Barcode::poll();
    Sensor::update();
    Notification::poll();
    Database::loop();

    // --- Core business logic ---
    stateMachine.update();

    // --- Output / Feedback ---
    Display::update(stateMachine.currentState(), Uv::secondsRemaining());

    digitalWrite(LED_YELLOW_PIN, stateMachine.currentState() != SystemState::IDLE_LOCKED);
    digitalWrite(LED_RED_PIN, stateMachine.currentState() == SystemState::ALARM_TRIGGERED);

    // --- Periodic heartbeat to Firebase ---
    if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL_MS) {
        lastHeartbeat = millis();
        Database::sendHeartbeat("state_" + String((int)stateMachine.currentState()));
    }
}
