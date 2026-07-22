// =====================================================================
//  Parcel Guardian AI - ESP32-CAM #1 "Courier Cam"
//  ถ่ายภาพใบหน้า/ท่าทางคนส่งของ ทันทีหลัง barcode ผ่านการตรวจสอบ
//  รับ trigger ได้ 2 ทาง: HTTP GET /capture (หลัก) และ GPIO interrupt (backup)
// =====================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"
#include "camera_config.h"
#include "uploader.h"
#include "secrets.h"

WebServer server(80);
volatile bool gpioTriggerFlag = false;

void IRAM_ATTR onGpioTrigger() {
    gpioTriggerFlag = true;
}

static bool initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;   config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;   config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;   config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;   config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_SVGA; // 800x600 พอสำหรับหลักฐาน ไม่กิน bandwidth มาก
    config.jpeg_quality = 12;
    config.fb_count = 2;

    return esp_camera_init(&config) == ESP_OK;
}

static void handleCapture() {
    String tag = server.hasArg("tag") ? server.arg("tag") : "unknown";
    String url = Uploader::captureAndUpload(tag, "courier_photo_url");

    if (url.length() > 0) {
        server.send(200, "application/json", "{\"status\":\"ok\",\"url\":\"" + url + "\"}");
    } else {
        server.send(500, "application/json", "{\"status\":\"error\"}");
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(FLASH_LED_PIN, OUTPUT);
    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(TRIGGER_INPUT_PIN, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(TRIGGER_INPUT_PIN), onGpioTrigger, RISING);

    WiFi.mode(WIFI_STA);
    IPAddress ip, gw, sn;
    ip.fromString(STATIC_IP); gw.fromString(GATEWAY_IP); sn.fromString(SUBNET_MASK);
    WiFi.config(ip, gw, sn);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
    Serial.printf("\n[CourierCam] Connected, IP: %s\n", WiFi.localIP().toString().c_str());

    if (!initCamera()) {
        Serial.println("[CourierCam] Camera init FAILED");
    }

    Uploader::begin();

    server.on("/capture", HTTP_GET, handleCapture);
    server.begin();
    Serial.println("[CourierCam] HTTP server ready on /capture");
}

void loop() {
    server.handleClient();

    if (gpioTriggerFlag) {
        gpioTriggerFlag = false;
        Serial.println("[CourierCam] GPIO backup trigger received");
        Uploader::captureAndUpload("gpio_trigger", "courier_photo_url");
    }
}
