#pragma once
// UART barcode reader (GM65-compatible). Continuous auto-scan, RX-only.

#include <Arduino.h>

class BarcodeReader {
public:
    void begin();
    void poll();                 // call every loop; non-blocking
    bool hasCode() const;
    String takeCode();           // returns code and clears pending flag

private:
    String buffer_;
    String pending_;
    bool hasPending_ = false;

    void handleLine(const String& line);
};

extern BarcodeReader barcodeReader;
