#include "logger.h"
#include <stdarg.h>

static void printLine(const char* level, const char* tag, const char* msg) {
    Serial.printf("[%8lu][%s][%s] %s\n", millis(), level, tag, msg);
}

void Log::begin() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== Parcel Guardian AI - Main Controller ===");
}

void Log::info(const char* tag, const char* msg)  { printLine("INFO ", tag, msg); }
void Log::warn(const char* tag, const char* msg)  { printLine("WARN ", tag, msg); }
void Log::error(const char* tag, const char* msg) { printLine("ERROR", tag, msg); }

static void vformat(char* buf, size_t size, const char* fmt, va_list args) {
    vsnprintf(buf, size, fmt, args);
}

void Log::infof(const char* tag, const char* fmt, ...) {
    char buf[192];
    va_list args; va_start(args, fmt);
    vformat(buf, sizeof(buf), fmt, args);
    va_end(args);
    printLine("INFO ", tag, buf);
}

void Log::warnf(const char* tag, const char* fmt, ...) {
    char buf[192];
    va_list args; va_start(args, fmt);
    vformat(buf, sizeof(buf), fmt, args);
    va_end(args);
    printLine("WARN ", tag, buf);
}

void Log::errorf(const char* tag, const char* fmt, ...) {
    char buf[192];
    va_list args; va_start(args, fmt);
    vformat(buf, sizeof(buf), fmt, args);
    va_end(args);
    printLine("ERROR", tag, buf);
}
