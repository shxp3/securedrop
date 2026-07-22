#pragma once
// =====================================================================
// state_machine.h : แกนกลางของระบบ (Finite State Machine)
// อ้างอิง State Diagram ใน docs/01_Architecture_and_Flow.md หัวข้อ 1.5
// =====================================================================
#include <Arduino.h>

enum class SystemState {
    IDLE_LOCKED,              // สถานะปกติ กล่องล็อก เฝ้าระวังขโมย (ARMED)
    SCANNING,                 // กำลังตรวจสอบ tracking number กับ Firebase
    UNLOCKED_WAITING_COURIER, // ปลดล็อกรอคนส่งวางพัสดุ
    WAIT_PARCEL_DETECT,       // ประตูปิดแล้ว รอ ultrasonic ยืนยันพัสดุ
    LOCKING,                  // สั่งล็อกกลับ
    UV_ACTIVE,                // กำลังฉาย UV-C (Safety-Critical State)
    NOTIFYING,                // กำลังส่งข้อมูลไป Firebase/Telegram
    ALARM_TRIGGERED,          // ตรวจพบความพยายามขโมย (priority สูงสุด)
    TIMEOUT_ABORT             // ยกเลิกเนื่องจาก timeout
};

class StateMachine {
public:
    void begin();
    void update();                    // เรียกทุก loop() iteration
    SystemState currentState() const { return _state; }
    void forceAlarm();                // เรียกจาก sensor module เมื่อพบความผิดปกติ
    void acknowledgeAlarm();          // เรียกจาก Telegram /ack command

private:
    SystemState _state = SystemState::IDLE_LOCKED;
    unsigned long _stateEnteredAt = 0;
    String _currentTracking;          // tracking number ที่กำลังประมวลผลอยู่

    void transitionTo(SystemState newState);
    unsigned long elapsedInState() const { return millis() - _stateEnteredAt; }

    // handlers ต่อ state
    void handleIdleLocked();
    void handleScanning();
    void handleUnlockedWaitingCourier();
    void handleWaitParcelDetect();
    void handleLocking();
    void handleUvActive();
    void handleNotifying();
    void handleAlarmTriggered();
    void handleTimeoutAbort();
};

extern StateMachine stateMachine;
