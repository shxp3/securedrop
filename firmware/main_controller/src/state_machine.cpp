#include "state_machine.h"
#include "config.h"
#include "barcode.h"
#include "lock.h"
#include "telegram.h"
#include "wifi_manager.h"

StateMachine stateMachine;

const char* StateMachine::stateName() const {
    switch (state_) {
        case SystemState::BOOT:          return "BOOT";
        case SystemState::CONNECT_WIFI:  return "CONNECT_WIFI";
        case SystemState::READY:         return "READY";
        case SystemState::WAIT_BARCODE:  return "WAIT_BARCODE";
        case SystemState::VERIFY:        return "VERIFY";
        case SystemState::VALID:         return "VALID";
        case SystemState::UNLOCK:        return "UNLOCK";
        case SystemState::CAPTURE:       return "CAPTURE";
        case SystemState::SEND_TELEGRAM: return "SEND_TELEGRAM";
        case SystemState::LOCK:          return "LOCK";
        case SystemState::ERROR:         return "ERROR";
    }
    return "UNKNOWN";
}

void StateMachine::enter(SystemState next) {
    state_ = next;
    stateEnteredMs_ = millis();
    Serial.printf("[FSM] -> %s\n", stateName());
}

void StateMachine::begin() {
    enter(SystemState::BOOT);
}

void StateMachine::update() {
    switch (state_) {
        case SystemState::BOOT:          onBoot(); break;
        case SystemState::CONNECT_WIFI:  onConnectWifi(); break;
        case SystemState::READY:         onReady(); break;
        case SystemState::WAIT_BARCODE:  onWaitBarcode(); break;
        case SystemState::VERIFY:        onVerify(); break;
        case SystemState::VALID:         onValid(); break;
        case SystemState::UNLOCK:        onUnlock(); break;
        case SystemState::CAPTURE:       onCapture(); break;
        case SystemState::SEND_TELEGRAM: onSendTelegram(); break;
        case SystemState::LOCK:          onLock(); break;
        case SystemState::ERROR:         onError(); break;
    }
}

void StateMachine::onBoot() {
    enter(SystemState::CONNECT_WIFI);
}

void StateMachine::onConnectWifi() {
    if (wifiManager.isConnected()) {
        // Sync clock for Telegram timestamps
        configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC,
                   "pool.ntp.org", "time.nist.gov");
        enter(SystemState::READY);
        return;
    }
    // wifiManager.loop() handles retries from main.cpp
}

void StateMachine::onReady() {
    // Drain any stale scans accumulated during boot/network setup
    while (barcodeReader.hasCode()) {
        (void)barcodeReader.takeCode();
    }
    barcodeReader.disarm();
    image_.release();
    currentTracking_ = "";
    enter(SystemState::WAIT_BARCODE);
}

void StateMachine::onWaitBarcode() {
    // Keep a scan window open; trigger modes re-arm after timeout/cooldown.
    if (barcodeReader.state() == BarcodeState::Idle ||
        barcodeReader.state() == BarcodeState::Timeout ||
        barcodeReader.state() == BarcodeState::Error) {
        barcodeReader.arm();
    }

    if (!barcodeReader.hasCode()) {
        return;
    }
    currentTracking_ = barcodeReader.takeCode();
    barcodeReader.disarm();
    enter(SystemState::VERIFY);
}

void StateMachine::onVerify() {
    lastVerify_ = localDatabase.verify(currentTracking_);
    switch (lastVerify_) {
        case VerifyResult::Valid:
            enter(SystemState::VALID);
            break;
        case VerifyResult::Unknown:
        case VerifyResult::AlreadyUsed:
            enter(SystemState::ERROR);
            break;
    }
}

void StateMachine::onValid() {
    // Do not mark used yet — wait until unlock sequence completes (onLock).
    // In-progress tracking is held in currentTracking_; READY drains extras.
    enter(SystemState::UNLOCK);
}

void StateMachine::onUnlock() {
    if (!lockController.isUnlocked()) {
        lockController.unlock();
    }
    if (millis() - stateEnteredMs_ >= UNLOCK_HOLD_MS) {
        enter(SystemState::CAPTURE);
    }
}

void StateMachine::onCapture() {
    image_.release();
    if (!cameraClient.capture(image_)) {
        Serial.println("[FSM] Capture failed — continue with text-only notify");
    }
    enter(SystemState::SEND_TELEGRAM);
}

void StateMachine::onSendTelegram() {
    const String ts = telegramNotifier.nowTimestamp();
    const String caption =
        String("SecureDrop Delivery\n") +
        "Box: " + BOX_ID + "\n" +
        "Tracking: " + currentTracking_ + "\n" +
        "Time: " + ts + "\n" +
        "Status: VALID — unlocked & captured";

    if (image_.valid()) {
        telegramNotifier.sendPhoto(image_, caption);
    } else {
        telegramNotifier.sendText(caption + "\n(photo unavailable)");
    }

    image_.release();
    enter(SystemState::LOCK);
}

void StateMachine::onLock() {
    lockController.lock();
    // Mark used only after unlock → capture → notify completed (box was opened).
    if (currentTracking_.length() > 0) {
        localDatabase.markUsed(currentTracking_);
    }
    enter(SystemState::READY);
}

void StateMachine::onError() {
    String reason = "INVALID";
    if (lastVerify_ == VerifyResult::AlreadyUsed) {
        reason = "ALREADY USED";
    } else if (lastVerify_ == VerifyResult::Unknown) {
        reason = "UNKNOWN";
    }

    const String msg =
        String("SecureDrop ALERT\n") +
        "Box: " + BOX_ID + "\n" +
        "Tracking: " + currentTracking_ + "\n" +
        "Time: " + telegramNotifier.nowTimestamp() + "\n" +
        "Status: " + reason + " — lock NOT opened";

    telegramNotifier.sendText(msg);
    lockController.lock();  // ensure locked — never unlock on barcode error
    // Do not mark used on invalid / already-used paths.
    enter(SystemState::READY);
}
