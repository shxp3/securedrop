#pragma once
// =====================================================================
// SecureDrop v1 — non-secret configuration
// Secrets live in credentials.h (copy from credentials.h.example).
// =====================================================================

#include "pins.h"

#define FW_VERSION          "1.0.0-v1"
#define BOX_ID              "SECUREDROP-001"

// ---- Barcode (GM65-compatible UART) ----
#define BARCODE_BAUD        9600
#define BARCODE_MAX_LEN     48

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
