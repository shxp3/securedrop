#pragma once
// =====================================================================
// SecureDrop v1 — GPIO pin map (Main Controller)
// Change pins here only; do not hardcode GPIO numbers in modules.
// =====================================================================

// Scanner TXD → ESP32 RX (UART2).
// Scanner RXD ← ESP32 TX — required only for UART_TRIGGER / config commands.
static const int PIN_BARCODE_RX = 16;
static const int PIN_BARCODE_TX = 17;  // unused unless BARCODE_SCAN_MODE needs TX

// Optional hardware TRIG pin on the scanner module.
// Set to a free GPIO only after confirming the module exposes TRIG and its polarity.
// -1 = not wired / not used (HARDWARE_TRIGGER will log ERROR until assigned).
static const int PIN_BARCODE_TRIGGER = -1;
static const bool BARCODE_TRIGGER_ACTIVE_LOW = true;  // most modules: TRIG asserted LOW

// Relay IN → drives 12V Fail-Secure solenoid (Active-LOW module)
static const int PIN_RELAY = 25;
static const bool RELAY_ACTIVE_LOW = true;
