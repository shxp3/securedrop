#include "barcode.h"
#include "config.h"
#include "pins.h"

HardwareSerial BarcodeSerial(2);

BarcodeReader barcodeReader;

void BarcodeReader::begin() {
    buffer_.reserve(BARCODE_MAX_LEN);
    BarcodeSerial.begin(BARCODE_BAUD, SERIAL_8N1, PIN_BARCODE_RX, PIN_BARCODE_TX);
    Serial.printf("[Barcode] UART2 RX=GPIO%d baud=%d (RX-only)\n",
                  PIN_BARCODE_RX, BARCODE_BAUD);
}

void BarcodeReader::handleLine(const String& line) {
    String code = line;
    code.trim();
    if (code.length() == 0) {
        return;
    }
    // Ignore noisy fragments that are clearly not tracking IDs
    if (code.length() < 4 || code.length() > BARCODE_MAX_LEN) {
        Serial.printf("[Barcode] Ignored (len=%u): %s\n",
                      static_cast<unsigned>(code.length()), code.c_str());
        return;
    }
    pending_ = code;
    hasPending_ = true;
    Serial.printf("[Barcode] Scanned: %s\n", pending_.c_str());
}

void BarcodeReader::poll() {
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
            // Overflow protection: reset line
            buffer_ = "";
            Serial.println("[Barcode] Line overflow, buffer reset");
        }
    }
}

bool BarcodeReader::hasCode() const {
    return hasPending_;
}

String BarcodeReader::takeCode() {
    hasPending_ = false;
    String out = pending_;
    pending_ = "";
    return out;
}
