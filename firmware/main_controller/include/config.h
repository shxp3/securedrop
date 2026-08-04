#pragma once
// =====================================================================
// SecureDrop v1 — non-secret configuration
// Secrets live in credentials.h (copy from credentials.h.example).
// =====================================================================

#include "pins.h"

#define FW_VERSION          "1.1.0-v1"
#define BOX_ID              "SECUREDROP-001"

// ---- Barcode scan modes ----
// CONTINUOUS      — scanner always decoding (illumination usually always on)
// PRESENTATION    — scanner lights/decodes only when a code is presented
//                   (configure via manufacturer setup barcodes; no UART cmds)
// UART_TRIGGER    — ESP32 sends start/stop commands over UART (needs TX wire
//                   + confirmed command bytes — see barcode_commands.h)
// HARDWARE_TRIGGER — ESP32 asserts PIN_BARCODE_TRIGGER (needs TRIG pin)
enum class BarcodeScanMode : uint8_t {
    Continuous = 0,
    Presentation = 1,
    UartTrigger = 2,
    HardwareTrigger = 3
};

// Preferred default: presentation/induction on the module itself.
// If the lamp stays on, reconfigure the scanner with its setup barcodes
// or switch mode after confirming UART/hardware trigger protocol.
#ifndef BARCODE_SCAN_MODE
#define BARCODE_SCAN_MODE   BarcodeScanMode::Presentation
#endif

// ---- Barcode (UART) ----
#define BARCODE_BAUD            9600
#define BARCODE_MAX_LEN         48
#define BARCODE_SCAN_TIMEOUT_MS 15000  // arm window → timeout (trigger modes)
#define BARCODE_COOLDOWN_MS     2500   // ignore new arms/reads after a scan
#define BARCODE_DUPLICATE_MS    5000   // ignore same code within this window

// ---- Lock timing ----
#define UNLOCK_HOLD_MS      5000   // unlock, wait, then capture

// ---- Camera (HTTP trigger, fixed IP) ----
#define CAM_HOST            "192.168.1.101"
#define CAM_PORT            80
#define CAM_CAPTURE_PATH    "/capture"
#define CAM_HTTP_TIMEOUT_MS 8000
#define CAM_MAX_JPEG_BYTES  65536  // safety cap for Main RAM

// ---- WiFi ----
#define WIFI_CONNECT_TIMEOUT_MS 20000
#define WIFI_RECONNECT_INTERVAL_MS 10000

// ---- Time (Thailand UTC+7) ----
#define NTP_GMT_OFFSET_SEC  (7 * 3600)
#define NTP_DAYLIGHT_OFFSET_SEC 0

// ---- Telegram ----
#define TELEGRAM_HTTP_TIMEOUT_MS 15000
