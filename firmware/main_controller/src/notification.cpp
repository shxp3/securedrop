#include "notification.h"
#include "config.h"
#include "logger.h"
#include "database.h"
#include "state_machine.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "secrets.h"

static WiFiClientSecure secureClient;
static UniversalTelegramBot bot(TELEGRAM_BOT_TOKEN, secureClient);
static unsigned long lastPollTime = 0;
static const unsigned long POLL_INTERVAL_MS = 2000;

void Notification::begin() {
    secureClient.setInsecure(); // ระดับ production ควรใช้ certificate fingerprint ที่แท้จริง
    Log::info("Notification", "Telegram bot initialized");
}

static String getCurrentTimeString() {
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", t->tm_hour, t->tm_min);
    return String(buf);
}

void Notification::sendDeliveryReport(const String& trackingNumber) {
    String msg = "📦 *Parcel Delivered*\n\n";
    msg += "Tracking:\n" + trackingNumber + "\n\n";
    msg += "Time:\n" + getCurrentTimeString() + "\n\n";
    msg += "Status:\nDelivered ✅\n\nUV-C Sterilization: Completed";

    bot.sendMessage(TELEGRAM_CHAT_ID, msg, "Markdown");
    // รูปคนส่ง/รูปพัสดุถูกอัปโหลดขึ้น Firebase Storage โดย ESP32-CAM โดยตรงแล้ว
    // ที่นี่ส่ง URL ต่อผ่าน sendPhoto ถ้าต้องการแนบภาพจริงในแชท (bot.sendPhoto(chat_id, url))
    Log::infof("Notification", "Delivery report sent for %s", trackingNumber.c_str());
}

void Notification::sendTheftAlert() {
    String msg = "⚠️ *ALERT — Possible Theft*\n\n";
    msg += "Parcel Box Movement Detected\n";
    msg += "Time: " + getCurrentTimeString() + "\n\n";
    msg += "ระบบได้ล็อกกล่องและเปิดไซเรนแล้ว\nส่งคำสั่ง /ack เพื่อปิดการแจ้งเตือน";

    bot.sendMessage(TELEGRAM_CHAT_ID, msg, "Markdown");
    Log::warn("Notification", "Theft alert sent");
}

void Notification::sendTimeoutNotice(const String& trackingNumber) {
    String msg = "⏱ *Timeout Notice*\n\n";
    msg += "Tracking: " + trackingNumber + "\n";
    msg += "ไม่พบการวางพัสดุหรือปิดประตูภายในเวลาที่กำหนด\nกล่องถูกล็อกกลับสู่สถานะปกติแล้ว";

    bot.sendMessage(TELEGRAM_CHAT_ID, msg, "Markdown");
    Log::warn("Notification", "Timeout notice sent");
}

void Notification::poll() {
    unsigned long now = millis();
    if (now - lastPollTime < POLL_INTERVAL_MS) return;
    lastPollTime = now;

    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    for (int i = 0; i < numNewMessages; i++) {
        String chatId = bot.messages[i].chat_id;
        String text = bot.messages[i].text;

        if (chatId != TELEGRAM_CHAT_ID) continue; // เฉพาะ chat_id ที่ลงทะเบียนไว้เท่านั้น

        if (text == "/ack") {
            stateMachine.acknowledgeAlarm();
            bot.sendMessage(chatId, "✅ Alarm acknowledged and reset.", "");
        } else if (text == "/status") {
            String s = "Box ID: " BOX_ID "\nState: " + String((int)stateMachine.currentState());
            bot.sendMessage(chatId, s, "");
        } else if (text.startsWith("/openbox")) {
            String pin = text.substring(text.indexOf(' ') + 1);
            if (pin == ADMIN_PIN) {
                bot.sendMessage(chatId, "🔓 Emergency unlock granted.", "");
                // เรียก Lock::unlock() ผ่านโมดูล lock ในระดับ integration จริง
            } else {
                bot.sendMessage(chatId, "❌ Invalid PIN.", "");
            }
        }
    }
}
