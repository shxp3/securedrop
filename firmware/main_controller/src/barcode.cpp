#include "barcode.h"
#include "config.h"
#include "logger.h"

static HardwareSerial barcodeSerial(2); // UART2
static String buffer;
static String pendingTracking;
static bool newScanFlag = false;

void Barcode::begin() {
    barcodeSerial.begin(BARCODE_BAUD, SERIAL_8N1, BARCODE_RX_PIN, BARCODE_TX_PIN);
    buffer.reserve(BARCODE_MAX_LEN + 4);
    Log::info("Barcode", "GM65/GM66 UART initialized");
}

bool Barcode::isValidFormat(const String& code) {
    if (code.length() < 4 || code.length() > BARCODE_MAX_LEN) return false;
    for (size_t i = 0; i < code.length(); i++) {
        char c = code[i];
        if (!isalnum(c)) return false;
    }
    return true;
}

void Barcode::poll() {
    while (barcodeSerial.available()) {
        char c = barcodeSerial.read();
        if (c == '\r' || c == '\n') {
            if (buffer.length() > 0) {
                String code = buffer;
                code.trim();
                buffer = "";
                if (isValidFormat(code)) {
                    pendingTracking = code;
                    newScanFlag = true;
                    Log::infof("Barcode", "Valid scan: %s", code.c_str());
                } else {
                    Log::warnf("Barcode", "Invalid format ignored: %s", code.c_str());
                }
            }
        } else {
            if (buffer.length() < BARCODE_MAX_LEN) buffer += c;
        }
    }
}

bool Barcode::hasNewScan() { return newScanFlag; }

String Barcode::readTracking() {
    newScanFlag = false;
    return pendingTracking;
}
