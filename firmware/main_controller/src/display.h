#pragma once
// display.h : (ตัวเลือก) แสดงสถานะระบบบน OLED SSD1306
#include <Arduino.h>
#include "state_machine.h"

namespace Display {
    void begin();
    void update(SystemState state, int uvSecondsRemaining);
}
