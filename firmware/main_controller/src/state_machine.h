#pragma once
// Finite state machine — SecureDrop v1 delivery flow.

#include <Arduino.h>
#include "camera.h"
#include "database.h"

enum class SystemState {
    BOOT,
    CONNECT_WIFI,
    READY,
    WAIT_BARCODE,
    VERIFY,
    VALID,
    UNLOCK,
    CAPTURE,
    SEND_TELEGRAM,
    LOCK,
    ERROR
};

class StateMachine {
public:
    void begin();
    void update();
    SystemState state() const { return state_; }
    const char* stateName() const;

private:
    SystemState state_ = SystemState::BOOT;
    String currentTracking_;
    CapturedImage image_;
    unsigned long stateEnteredMs_ = 0;
    VerifyResult lastVerify_ = VerifyResult::Unknown;

    void enter(SystemState next);
    void onBoot();
    void onConnectWifi();
    void onReady();
    void onWaitBarcode();
    void onVerify();
    void onValid();
    void onUnlock();
    void onCapture();
    void onSendTelegram();
    void onLock();
    void onError();
};

extern StateMachine stateMachine;
