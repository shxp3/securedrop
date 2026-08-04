// =====================================================================
// SecureDrop v1 — Main Controller
// Boots peripherals, then runs the delivery FSM every loop().
// =====================================================================

#include <Arduino.h>
#include "config.h"
#include "credentials.h"
#include "wifi_manager.h"
#include "barcode.h"
#include "lock.h"
#include "database.h"
#include "camera.h"
#include "telegram.h"
#include "state_machine.h"

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.printf("=== SecureDrop Main v%s ===\n", FW_VERSION);
    Serial.printf("Box ID: %s\n", BOX_ID);

    lockController.begin();       // locked first (fail-secure)
    barcodeReader.begin();
    localDatabase.begin();
    telegramNotifier.begin();

    wifiManager.begin();
    stateMachine.begin();
}

void loop() {
    wifiManager.loop();
    barcodeReader.poll();
    stateMachine.update();
}
