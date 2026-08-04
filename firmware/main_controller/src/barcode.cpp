#include "barcode.h"
#include "barcode_commands.h"
#include "config.h"
#include "pins.h"

HardwareSerial BarcodeSerial(2);

BarcodeReader barcodeReader;

const char* BarcodeReader::modeName(BarcodeScanMode mode) {
    switch (mode) {
        case BarcodeScanMode::Continuous:      return "CONTINUOUS";
        case BarcodeScanMode::Presentation:    return "PRESENTATION";
        case BarcodeScanMode::UartTrigger:     return "UART_TRIGGER";
        case BarcodeScanMode::HardwareTrigger: return "HARDWARE_TRIGGER";
    }
    return "UNKNOWN";
}

const char* BarcodeReader::stateName() const {
    switch (state_) {
        case BarcodeState::Idle:         return "IDLE";
        case BarcodeState::Arming:       return "ARMING";
        case BarcodeState::Scanning:     return "SCANNING";
        case BarcodeState::CodeReceived: return "CODE_RECEIVED";
        case BarcodeState::Timeout:      return "TIMEOUT";
        case BarcodeState::Error:        return "ERROR";
    }
    return "UNKNOWN";
}

bool BarcodeReader::needsActiveTrigger() const {
    return mode_ == BarcodeScanMode::UartTrigger ||
           mode_ == BarcodeScanMode::HardwareTrigger;
}

bool BarcodeReader::usesScanTimeout() const {
    return needsActiveTrigger();
}

bool BarcodeReader::inCooldown() const {
    return millis() < cooldownUntilMs_;
}

void BarcodeReader::setState(BarcodeState next) {
    if (state_ == next) {
        return;
    }
    state_ = next;
    Serial.printf("[Barcode] state=%s\n", stateName());
}

void BarcodeReader::begin() {
    mode_ = BARCODE_SCAN_MODE;
    buffer_.reserve(BARCODE_MAX_LEN);
    pending_ = "";
    hasPending_ = false;
    lastCode_ = "";
    lastCodeMs_ = 0;
    cooldownUntilMs_ = 0;
    triggerArmed_ = false;
    state_ = BarcodeState::Idle;

    uartTxEnabled_ = (mode_ == BarcodeScanMode::UartTrigger) && (PIN_BARCODE_TX >= 0);
    const int txPin = uartTxEnabled_ ? PIN_BARCODE_TX : -1;
    BarcodeSerial.begin(BARCODE_BAUD, SERIAL_8N1, PIN_BARCODE_RX, txPin);

    if (mode_ == BarcodeScanMode::HardwareTrigger && PIN_BARCODE_TRIGGER >= 0) {
        pinMode(PIN_BARCODE_TRIGGER, OUTPUT);
        // Idle = not asserted
        digitalWrite(PIN_BARCODE_TRIGGER,
                     BARCODE_TRIGGER_ACTIVE_LOW ? HIGH : LOW);
    }

    if (txPin >= 0) {
        Serial.printf("[Barcode] mode=%s UART2 RX=GPIO%d TX=GPIO%d baud=%d\n",
                      modeName(mode_), PIN_BARCODE_RX, txPin, BARCODE_BAUD);
    } else {
        Serial.printf("[Barcode] mode=%s UART2 RX=GPIO%d TX=off baud=%d\n",
                      modeName(mode_), PIN_BARCODE_RX, BARCODE_BAUD);
    }
    Serial.printf("[Barcode] timeout=%ums cooldown=%ums duplicate=%ums\n",
                  static_cast<unsigned>(BARCODE_SCAN_TIMEOUT_MS),
                  static_cast<unsigned>(BARCODE_COOLDOWN_MS),
                  static_cast<unsigned>(BARCODE_DUPLICATE_MS));

    if (mode_ == BarcodeScanMode::Presentation) {
        Serial.println("[Barcode] PRESENTATION: configure scanner via manufacturer");
        Serial.println("[Barcode] setup barcodes so lamp/decode activate on present only.");
    }
    if (mode_ == BarcodeScanMode::UartTrigger) {
        if (BARCODE_CMD_START_SCAN_LEN == 0 || BARCODE_CMD_STOP_SCAN_LEN == 0) {
            Serial.println("[Barcode] UART_TRIGGER: command bytes not configured");
            Serial.println("[Barcode] Fill barcode_commands.h after confirming model.");
        }
        if (PIN_BARCODE_TX < 0) {
            Serial.println("[Barcode] UART_TRIGGER: PIN_BARCODE_TX not assigned");
        }
    }
    if (mode_ == BarcodeScanMode::HardwareTrigger && PIN_BARCODE_TRIGGER < 0) {
        Serial.println("[Barcode] HARDWARE_TRIGGER: PIN_BARCODE_TRIGGER not assigned");
    }
}

bool BarcodeReader::sendUartCommand(const uint8_t* data, size_t len, const char* tag) {
    if (!uartTxEnabled_ || data == nullptr || len == 0) {
        Serial.printf("[Barcode] UART cmd '%s' unavailable (no bytes / no TX)\n", tag);
        return false;
    }
    BarcodeSerial.write(data, len);
    BarcodeSerial.flush();
    Serial.printf("[Barcode] UART cmd '%s' sent (%u bytes)\n",
                  tag, static_cast<unsigned>(len));
    return true;
}

bool BarcodeReader::startHardwareTrigger() {
    if (PIN_BARCODE_TRIGGER < 0) {
        Serial.println("[Barcode] TRIG pin not assigned");
        return false;
    }
    digitalWrite(PIN_BARCODE_TRIGGER,
                 BARCODE_TRIGGER_ACTIVE_LOW ? LOW : HIGH);
    triggerArmed_ = true;
    Serial.printf("[Barcode] TRIG GPIO%d asserted\n", PIN_BARCODE_TRIGGER);
    return true;
}

void BarcodeReader::stopHardwareTrigger() {
    if (PIN_BARCODE_TRIGGER < 0) {
        return;
    }
    digitalWrite(PIN_BARCODE_TRIGGER,
                 BARCODE_TRIGGER_ACTIVE_LOW ? HIGH : LOW);
    triggerArmed_ = false;
    Serial.printf("[Barcode] TRIG GPIO%d deasserted\n", PIN_BARCODE_TRIGGER);
}

void BarcodeReader::arm() {
    if (hasPending_ || state_ == BarcodeState::CodeReceived) {
        return;
    }
    if (inCooldown()) {
        return;
    }
    if (state_ == BarcodeState::Scanning || state_ == BarcodeState::Arming) {
        return;
    }

    setState(BarcodeState::Arming);
    armStartedMs_ = millis();

    switch (mode_) {
        case BarcodeScanMode::Continuous:
        case BarcodeScanMode::Presentation:
            // Passive listen — illumination controlled by scanner configuration.
            setState(BarcodeState::Scanning);
            break;

        case BarcodeScanMode::UartTrigger:
            if (BARCODE_CMD_START_SCAN_LEN == 0 || !uartTxEnabled_) {
                Serial.println("[Barcode] Cannot arm UART_TRIGGER — missing config");
                cooldownUntilMs_ = millis() + BARCODE_COOLDOWN_MS;
                setState(BarcodeState::Error);
                return;
            }
            if (!sendUartCommand(BARCODE_CMD_START_SCAN,
                                 BARCODE_CMD_START_SCAN_LEN, "START_SCAN")) {
                cooldownUntilMs_ = millis() + BARCODE_COOLDOWN_MS;
                setState(BarcodeState::Error);
                return;
            }
            setState(BarcodeState::Scanning);
            break;

        case BarcodeScanMode::HardwareTrigger:
            if (!startHardwareTrigger()) {
                cooldownUntilMs_ = millis() + BARCODE_COOLDOWN_MS;
                setState(BarcodeState::Error);
                return;
            }
            setState(BarcodeState::Scanning);
            break;
    }
}

void BarcodeReader::disarm() {
    if (mode_ == BarcodeScanMode::UartTrigger &&
        (state_ == BarcodeState::Scanning || state_ == BarcodeState::Arming ||
         state_ == BarcodeState::Timeout)) {
        if (BARCODE_CMD_STOP_SCAN_LEN > 0 && uartTxEnabled_) {
            (void)sendUartCommand(BARCODE_CMD_STOP_SCAN,
                                  BARCODE_CMD_STOP_SCAN_LEN, "STOP_SCAN");
        }
    }
    if (mode_ == BarcodeScanMode::HardwareTrigger && triggerArmed_) {
        stopHardwareTrigger();
    }

    if (state_ != BarcodeState::CodeReceived && !hasPending_) {
        setState(BarcodeState::Idle);
    }
}

bool BarcodeReader::isDuplicate(const String& code) const {
    if (lastCode_.length() == 0) {
        return false;
    }
    if (!code.equalsIgnoreCase(lastCode_)) {
        return false;
    }
    return (millis() - lastCodeMs_) < BARCODE_DUPLICATE_MS;
}

void BarcodeReader::handleLine(const String& line) {
    String code = line;
    code.trim();
    if (code.length() == 0) {
        return;
    }
    if (code.length() < 4 || code.length() > BARCODE_MAX_LEN) {
        Serial.printf("[Barcode] Ignored (len=%u)\n",
                      static_cast<unsigned>(code.length()));
        return;
    }

    // Only accept while actively scanning (or continuous always-on path).
    if (state_ != BarcodeState::Scanning &&
        state_ != BarcodeState::Arming) {
        Serial.printf("[Barcode] Dropped while %s: (hidden)\n", stateName());
        return;
    }

    if (inCooldown() || isDuplicate(code)) {
        Serial.println("[Barcode] Duplicate/cooldown — ignored");
        return;
    }

    pending_ = code;
    hasPending_ = true;
    lastCode_ = code;
    lastCodeMs_ = millis();
    setState(BarcodeState::CodeReceived);
    Serial.printf("[Barcode] Scanned: %s\n", pending_.c_str());

    // Stop active triggers as soon as a code arrives.
    if (mode_ == BarcodeScanMode::UartTrigger && BARCODE_CMD_STOP_SCAN_LEN > 0) {
        (void)sendUartCommand(BARCODE_CMD_STOP_SCAN,
                              BARCODE_CMD_STOP_SCAN_LEN, "STOP_SCAN");
    }
    if (mode_ == BarcodeScanMode::HardwareTrigger && triggerArmed_) {
        stopHardwareTrigger();
    }
}

void BarcodeReader::readUart() {
    while (BarcodeSerial.available() > 0) {
        const char c = static_cast<char>(BarcodeSerial.read());
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            handleLine(buffer_);
            buffer_ = "";
            continue;
        }
        if (buffer_.length() < BARCODE_MAX_LEN) {
            buffer_ += c;
        } else {
            buffer_ = "";
            Serial.println("[Barcode] Line overflow, buffer reset");
        }
    }
}

void BarcodeReader::checkTimeout() {
    if (!usesScanTimeout()) {
        return;
    }
    if (state_ != BarcodeState::Scanning && state_ != BarcodeState::Arming) {
        return;
    }
    if (millis() - armStartedMs_ < BARCODE_SCAN_TIMEOUT_MS) {
        return;
    }

    Serial.println("[Barcode] Scan timeout");
    if (mode_ == BarcodeScanMode::UartTrigger && BARCODE_CMD_STOP_SCAN_LEN > 0) {
        (void)sendUartCommand(BARCODE_CMD_STOP_SCAN,
                              BARCODE_CMD_STOP_SCAN_LEN, "STOP_SCAN");
    }
    if (mode_ == BarcodeScanMode::HardwareTrigger && triggerArmed_) {
        stopHardwareTrigger();
    }
    cooldownUntilMs_ = millis() + BARCODE_COOLDOWN_MS;
    setState(BarcodeState::Timeout);
}

void BarcodeReader::checkCooldown() {
    if (state_ != BarcodeState::Timeout && state_ != BarcodeState::Error) {
        // After takeCode we enter Idle with cooldown; nothing else to do.
        return;
    }
    if (!inCooldown()) {
        setState(BarcodeState::Idle);
    }
}

void BarcodeReader::poll() {
    readUart();
    checkTimeout();
    checkCooldown();
}

bool BarcodeReader::hasCode() const {
    return hasPending_;
}

String BarcodeReader::takeCode() {
    hasPending_ = false;
    String out = pending_;
    pending_ = "";
    cooldownUntilMs_ = millis() + BARCODE_COOLDOWN_MS;
    setState(BarcodeState::Idle);
    return out;
}
