#pragma once
// =====================================================================
// Scanner UART command placeholders
//
// DO NOT invent command bytes.
// Fill these only after confirming the exact scanner model and
// datasheet / official UART protocol (MH-ET LIVE variant, GM65, etc.).
//
// Until then, lengths stay 0. UART_TRIGGER mode will refuse to arm
// and log an ERROR instead of sending fake commands.
//
// When ready, replace with e.g.:
//   static const uint8_t BARCODE_CMD_START_SCAN[] = { 0x7E, /* ... */ };
//   static constexpr size_t BARCODE_CMD_START_SCAN_LEN = sizeof(...);
// =====================================================================

#include <Arduino.h>

// Null + length 0 until a confirmed protocol is provided.
static const uint8_t* const BARCODE_CMD_START_SCAN = nullptr;
static constexpr size_t BARCODE_CMD_START_SCAN_LEN = 0;

static const uint8_t* const BARCODE_CMD_STOP_SCAN = nullptr;
static constexpr size_t BARCODE_CMD_STOP_SCAN_LEN = 0;
