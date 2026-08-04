#pragma once
// Configurable barcode driver: continuous / presentation / UART / hardware trigger.
// UART command bytes live in barcode_commands.h as placeholders until confirmed.

#include <Arduino.h>
#include "config.h"

enum class BarcodeState : uint8_t {
    Idle = 0,
    Arming = 1,
    Scanning = 2,
    CodeReceived = 3,
    Timeout = 4,
    Error = 5
};

class BarcodeReader {
public:
    void begin();
    void poll();                 // call every loop; non-blocking
    void arm();                  // open a scan window / accept reads
    void disarm();               // stop trigger / leave accept window
    bool hasCode() const;
    String takeCode();           // returns code and clears pending flag
    BarcodeState state() const { return state_; }
    const char* stateName() const;
    static const char* modeName(BarcodeScanMode mode);
    BarcodeScanMode mode() const { return mode_; }

private:
    BarcodeScanMode mode_ = BARCODE_SCAN_MODE;
    BarcodeState state_ = BarcodeState::Idle;

    String buffer_;
    String pending_;
    bool hasPending_ = false;

    String lastCode_;
    unsigned long lastCodeMs_ = 0;
    unsigned long armStartedMs_ = 0;
    unsigned long cooldownUntilMs_ = 0;

    bool uartTxEnabled_ = false;
    bool triggerArmed_ = false;

    void setState(BarcodeState next);
    void handleLine(const String& line);
    void readUart();
    void checkTimeout();
    void checkCooldown();
    bool inCooldown() const;
    bool isDuplicate(const String& code) const;
    bool sendUartCommand(const uint8_t* data, size_t len, const char* tag);
    bool startHardwareTrigger();
    void stopHardwareTrigger();
    bool needsActiveTrigger() const;
    bool usesScanTimeout() const;
};

extern BarcodeReader barcodeReader;
