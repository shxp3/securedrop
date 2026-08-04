#pragma once
// =====================================================================
// SecureDrop v1 — GPIO pin map (Main Controller)
// Change pins here only; do not hardcode GPIO numbers in modules.
// =====================================================================

// GM65 TXD → ESP32 RX (UART2). TX unused (scanner is RX-only).
static const int PIN_BARCODE_RX = 16;
static const int PIN_BARCODE_TX = -1;  // RX-only: no TX wire

// Relay IN → drives 12V Fail-Secure solenoid (Active-LOW module)
static const int PIN_RELAY = 25;
static const bool RELAY_ACTIVE_LOW = true;
