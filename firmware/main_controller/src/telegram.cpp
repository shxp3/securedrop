#include "telegram.h"
#include "config.h"
#include "credentials.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

TelegramNotifier telegramNotifier;

bool TelegramNotifier::begin() {
    Serial.println("[Telegram] Client ready");
    return true;
}

bool TelegramNotifier::ensureConnected() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Telegram] WiFi down");
        return false;
    }
    return true;
}

String TelegramNotifier::nowTimestamp() const {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 2000)) {
        return String("time-unavailable");
    }
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buf);
}

bool TelegramNotifier::sendText(const String& message) {
    if (!ensureConnected()) {
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(TELEGRAM_HTTP_TIMEOUT_MS / 1000);

    HTTPClient http;
    http.setTimeout(TELEGRAM_HTTP_TIMEOUT_MS);
    const String url = String("https://api.telegram.org/bot") + TELEGRAM_BOT_TOKEN + "/sendMessage";

    if (!http.begin(client, url)) {
        Serial.println("[Telegram] begin(sendMessage) failed");
        return false;
    }

    http.addHeader("Content-Type", "application/json");

    // Minimal JSON escaping for quotes/newlines in message
    String escaped = message;
    escaped.replace("\\", "\\\\");
    escaped.replace("\"", "\\\"");
    escaped.replace("\n", "\\n");

    const String body = String("{\"chat_id\":\"") + TELEGRAM_CHAT_ID +
                        "\",\"text\":\"" + escaped + "\"}";

    const int code = http.POST(body);
    const String resp = http.getString();
    http.end();

    const bool ok = (code == 200) && (resp.indexOf("\"ok\":true") >= 0);
    Serial.printf("[Telegram] sendMessage HTTP %d %s\n", code, ok ? "OK" : "FAIL");
    if (!ok) {
        Serial.println(resp.substring(0, min<unsigned>(resp.length(), 180)));
    }
    return ok;
}

bool TelegramNotifier::sendPhoto(const CapturedImage& image, const String& caption) {
    if (!ensureConnected()) {
        return false;
    }
    if (!image.valid()) {
        return sendText(caption + "\n(photo unavailable)");
    }

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(TELEGRAM_HTTP_TIMEOUT_MS / 1000);

    if (!client.connect("api.telegram.org", 443)) {
        Serial.println("[Telegram] TLS connect failed");
        sendText(caption + "\n(photo upload failed: TLS)");
        return false;
    }

    const String boundary = "SecureDropBoundary7Aa";
    const String head =
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" +
        String(TELEGRAM_CHAT_ID) + "\r\n" +
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"caption\"\r\n\r\n" +
        caption + "\r\n" +
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"photo\"; filename=\"courier.jpg\"\r\n"
        "Content-Type: image/jpeg\r\n\r\n";

    const String tail = "\r\n--" + boundary + "--\r\n";
    const size_t contentLength = head.length() + image.length + tail.length();

    client.printf(
        "POST /bot%s/sendPhoto HTTP/1.1\r\n"
        "Host: api.telegram.org\r\n"
        "Connection: close\r\n"
        "Content-Type: multipart/form-data; boundary=%s\r\n"
        "Content-Length: %u\r\n"
        "\r\n",
        TELEGRAM_BOT_TOKEN, boundary.c_str(),
        static_cast<unsigned>(contentLength));

    client.print(head);

    size_t sent = 0;
    while (sent < image.length) {
        const size_t n = client.write(image.data + sent, image.length - sent);
        if (n == 0) {
            Serial.println("[Telegram] Photo write stalled");
            client.stop();
            sendText(caption + "\n(photo upload failed: write)");
            return false;
        }
        sent += n;
    }

    client.print(tail);

    // Read status line / small body for logging
    String status = client.readStringUntil('\n');
    status.trim();
    String body;
    unsigned long start = millis();
    while (client.connected() && millis() - start < TELEGRAM_HTTP_TIMEOUT_MS) {
        while (client.available()) {
            body += static_cast<char>(client.read());
            if (body.length() > 512) {
                break;
            }
            start = millis();
        }
        if (body.length() > 512) {
            break;
        }
        delay(1);
    }
    client.stop();

    const bool ok = (status.indexOf("200") >= 0) && (body.indexOf("\"ok\":true") >= 0);
    Serial.printf("[Telegram] sendPhoto %s (%s)\n", ok ? "OK" : "FAIL", status.c_str());
    if (!ok) {
        Serial.println(body.substring(0, min<unsigned>(body.length(), 180)));
        sendText(caption + "\n(photo upload failed)");
    }
    return ok;
}
