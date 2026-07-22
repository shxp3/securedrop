#include "state_machine.h"
#include "config.h"
#include "barcode.h"
#include "database.h"
#include "camera_link.h"
#include "notification.h"
#include "lock.h"
#include "sensor.h"
#include "alarm.h"
#include "uv.h"
#include "logger.h"

StateMachine stateMachine;

void StateMachine::begin() {
    _state = SystemState::IDLE_LOCKED;
    _stateEnteredAt = millis();
    Log::info("StateMachine", "Boot -> IDLE_LOCKED (ARMED)");
}

void StateMachine::transitionTo(SystemState newState) {
    Log::infof("StateMachine", "Transition: %d -> %d", (int)_state, (int)newState);
    _state = newState;
    _stateEnteredAt = millis();
}

void StateMachine::forceAlarm() {
    // Alarm มี priority สูงสุด สามารถ interrupt ได้ทุก state
    // ข้อยกเว้นเดียว: ถ้ากำลัง UV_ACTIVE ต้องตัด UV ก่อนเสมอ (safety) แล้วค่อยเข้า alarm
    if (_state == SystemState::UV_ACTIVE) {
        Uv::forceOff("Alarm triggered during UV cycle");
    }
    transitionTo(SystemState::ALARM_TRIGGERED);
}

void StateMachine::acknowledgeAlarm() {
    if (_state == SystemState::ALARM_TRIGGERED) {
        Alarm::stop();
        transitionTo(SystemState::IDLE_LOCKED);
    }
}

void StateMachine::update() {
    // --- Safety-critical global check: ทำก่อนทุกอย่างในทุก loop ---
    // ถ้า UV กำลังทำงานแต่ประตูเปิด -> ตัดทันที ไม่ว่า state ปัจจุบันคืออะไร
    if (_state == SystemState::UV_ACTIVE && Lock::isDoorOpen()) {
        Uv::forceOff("Door opened during UV cycle - SAFETY VIOLATION");
        Log::error("StateMachine", "SAFETY: Door opened during UV, forced OFF");
        transitionTo(SystemState::NOTIFYING);
        return;
    }

    // --- Sensor Fusion ตรวจจับขโมยตลอดเวลา ยกเว้นระหว่างขั้นตอนกำลัง handshake กับคนส่ง ---
    if (_state != SystemState::ALARM_TRIGGERED &&
        _state != SystemState::UNLOCKED_WAITING_COURIER &&
        Sensor::isTheftDetected()) {
        forceAlarm();
    }

    switch (_state) {
        case SystemState::IDLE_LOCKED:              handleIdleLocked(); break;
        case SystemState::SCANNING:                 handleScanning(); break;
        case SystemState::UNLOCKED_WAITING_COURIER:  handleUnlockedWaitingCourier(); break;
        case SystemState::WAIT_PARCEL_DETECT:        handleWaitParcelDetect(); break;
        case SystemState::LOCKING:                   handleLocking(); break;
        case SystemState::UV_ACTIVE:                 handleUvActive(); break;
        case SystemState::NOTIFYING:                 handleNotifying(); break;
        case SystemState::ALARM_TRIGGERED:           handleAlarmTriggered(); break;
        case SystemState::TIMEOUT_ABORT:             handleTimeoutAbort(); break;
    }
}

void StateMachine::handleIdleLocked() {
    if (Barcode::hasNewScan()) {
        _currentTracking = Barcode::readTracking();
        Log::infof("StateMachine", "Barcode scanned: %s", _currentTracking.c_str());
        transitionTo(SystemState::SCANNING);
    }
}

void StateMachine::handleScanning() {
    Database::ParcelRecord record;
    bool found = Database::lookupParcel(_currentTracking, record);

    if (!found || record.used) {
        Log::warn("StateMachine", found ? "Tracking already used" : "Tracking not found");
        Alarm::beepShort();
        transitionTo(SystemState::IDLE_LOCKED);
        return;
    }

    Lock::unlock();
    CameraLink::triggerCourierCam(_currentTracking);
    transitionTo(SystemState::UNLOCKED_WAITING_COURIER);
}

void StateMachine::handleUnlockedWaitingCourier() {
    if (Lock::justClosedDoor()) {
        transitionTo(SystemState::WAIT_PARCEL_DETECT);
        return;
    }
    if (elapsedInState() > (UNLOCK_TIMEOUT_SEC * 1000UL)) {
        Log::warn("StateMachine", "Unlock timeout - courier did not close door in time");
        transitionTo(SystemState::TIMEOUT_ABORT);
    }
}

void StateMachine::handleWaitParcelDetect() {
    if (Sensor::isParcelDetected()) {
        transitionTo(SystemState::LOCKING);
        return;
    }
    if (elapsedInState() > (PARCEL_DETECT_TIMEOUT_SEC * 1000UL)) {
        Log::warn("StateMachine", "No parcel detected within timeout");
        Database::updateParcelStatus(_currentTracking, "no_parcel_detected", false);
        transitionTo(SystemState::TIMEOUT_ABORT);
    }
}

void StateMachine::handleLocking() {
    Lock::lock();
    CameraLink::triggerInternalCam(_currentTracking);

    // Safety Interlock: เข้าสู่ UV ได้ก็ต่อเมื่อ door=CLOSED AND parcel=DETECTED เท่านั้น
    if (!Lock::isDoorOpen() && Sensor::isParcelDetected()) {
        Uv::start(UV_DURATION_SEC);
        transitionTo(SystemState::UV_ACTIVE);
    } else {
        Log::error("StateMachine", "Safety interlock failed - skip UV");
        transitionTo(SystemState::NOTIFYING);
    }
}

void StateMachine::handleUvActive() {
    if (Uv::isComplete()) {
        transitionTo(SystemState::NOTIFYING);
    }
    // การตรวจ door-open ระหว่าง UV ทำที่ระดับ global check ด้านบนแล้ว (สูงสุด priority)
}

void StateMachine::handleNotifying() {
    Database::updateParcelStatus(_currentTracking, "delivered", true);
    Notification::sendDeliveryReport(_currentTracking);
    _currentTracking = "";
    transitionTo(SystemState::IDLE_LOCKED);
}

void StateMachine::handleAlarmTriggered() {
    Alarm::trigger();
    CameraLink::triggerCourierCam("THEFT_EVENT");
    CameraLink::triggerInternalCam("THEFT_EVENT");
    Notification::sendTheftAlert();
    Lock::lock();  // ล็อกกล่องกันงัดเพิ่ม
    // รอ acknowledgeAlarm() ถูกเรียกจาก Telegram /ack command
}

void StateMachine::handleTimeoutAbort() {
    Lock::lock();
    Notification::sendTimeoutNotice(_currentTracking);
    _currentTracking = "";
    transitionTo(SystemState::IDLE_LOCKED);
}
