// =====================================================================
// SecureDrop v1 — ESP32-CAM
// Serves GET /capture → image/jpeg for the Main Controller.
// No Firebase. No SD. No GPIO trigger.
// =====================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"
#include "camera_pins.h"
#include "credentials.h"

WebServer server(80);

static bool initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;
    config.pin_d0       = Y2_GPIO_NUM;
    config.pin_d1       = Y3_GPIO_NUM;
    config.pin_d2       = Y4_GPIO_NUM;
    config.pin_d3       = Y5_GPIO_NUM;
    config.pin_d4       = Y6_GPIO_NUM;
    config.pin_d5       = Y7_GPIO_NUM;
    config.pin_d6       = Y8_GPIO_NUM;
    config.pin_d7       = Y9_GPIO_NUM;
    config.pin_xclk     = XCLK_GPIO_NUM;
    config.pin_pclk     = PCLK_GPIO_NUM;
    config.pin_vsync    = VSYNC_GPIO_NUM;
    config.pin_href     = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn     = PWDN_GPIO_NUM;
    config.pin_reset    = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    // VGA keeps Main Controller RAM usage manageable
    if (psramFound()) {
        config.frame_size   = FRAMESIZE_VGA;
        config.jpeg_quality = 12;
        config.fb_count     = 2;
        config.fb_location  = CAMERA_FB_IN_PSRAM;
    } else {
        config.frame_size   = FRAMESIZE_QVGA;
        config.jpeg_quality = 15;
        config.fb_count     = 1;
        config.fb_location  = CAMERA_FB_IN_DRAM;
    }

    const esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("[CAM] init failed: 0x%x\n", err);
        return false;
    }
    return true;
}

static void handleRoot() {
    server.send(200, "text/plain", "SecureDrop ESP32-CAM OK\nGET /capture\n");
}

static void handleCapture() {
    // Brief flash for indoor demo lighting
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(80);

    camera_fb_t* fb = esp_camera_fb_get();
    digitalWrite(FLASH_LED_PIN, LOW);

    if (fb == nullptr) {
        Serial.println("[CAM] Capture failed");
        server.send(500, "text/plain", "capture failed");
        return;
    }

    Serial.printf("[CAM] JPEG %u bytes\n", static_cast<unsigned>(fb->len));

    WiFiClient client = server.client();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: image/jpeg");
    client.println("Content-Disposition: inline; filename=courier.jpg");
    client.print("Content-Length: ");
    client.println(fb->len);
    client.println("Connection: close");
    client.println();
    client.write(fb->buf, fb->len);
    client.flush();

    esp_camera_fb_return(fb);
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== SecureDrop ESP32-CAM v1 ===");

    pinMode(FLASH_LED_PIN, OUTPUT);
    digitalWrite(FLASH_LED_PIN, LOW);

    WiFi.mode(WIFI_STA);
    IPAddress ip, gw, sn;
    ip.fromString(CAM_STATIC_IP);
    gw.fromString(CAM_GATEWAY);
    sn.fromString(CAM_SUBNET);
    WiFi.config(ip, gw, sn);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.printf("[CAM] Connecting WiFi '%s'...\n", WIFI_SSID);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 30000) {
        delay(300);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[CAM] WiFi failed — restarting in 5s");
        delay(5000);
        ESP.restart();
    }

    Serial.printf("[CAM] IP %s\n", WiFi.localIP().toString().c_str());

    if (!initCamera()) {
        Serial.println("[CAM] Camera init failed — restarting in 5s");
        delay(5000);
        ESP.restart();
    }

    server.on("/", HTTP_GET, handleRoot);
    server.on("/capture", HTTP_GET, handleCapture);
    server.begin();
    Serial.println("[CAM] HTTP server ready: GET /capture");
}

void loop() {
    server.handleClient();
}
