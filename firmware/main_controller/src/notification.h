#pragma once
// notification.h : รวมศูนย์การส่งข้อความแจ้งเตือน (Telegram Bot) - เปลี่ยนเป็น LINE ในอนาคตแก้ที่นี่ที่เดียว
#include <Arduino.h>

namespace Notification {
    void begin();
    void poll();  // เช็ค incoming commands เช่น /ack /status /openbox

    void sendDeliveryReport(const String& trackingNumber);
    void sendTheftAlert();
    void sendTimeoutNotice(const String& trackingNumber);
}
