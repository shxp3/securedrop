#pragma once
// logger.h : รวมศูนย์การ log ผ่าน Serial (และขยายไป SD Card ได้ในอนาคต)
#include <Arduino.h>

namespace Log {
    void begin();
    void info(const char* tag, const char* msg);
    void warn(const char* tag, const char* msg);
    void error(const char* tag, const char* msg);
    void infof(const char* tag, const char* fmt, ...);
    void warnf(const char* tag, const char* fmt, ...);
    void errorf(const char* tag, const char* fmt, ...);
}
