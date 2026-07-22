#pragma once
// uv.h : Safety-Critical Module - ควบคุม UV-C พร้อม Hard Interlock
// หมายเหตุ: Safety Interlock ที่แท้จริงต้องมีทางฮาร์ดแวร์ (door sensor ต่ออนุกรมกับวงจรไฟเลี้ยง relay)
// โมดูลนี้เป็น "ชั้นซอฟต์แวร์" ป้องกันซ้ำอีกชั้น (Defense in Depth) ไม่ใช่มาตรการเดียว
#include <Arduino.h>

namespace Uv {
    void begin();
    void start(int durationSec);
    void forceOff(const char* reason);
    bool isComplete();
    bool isActive();
    int secondsRemaining();
}
