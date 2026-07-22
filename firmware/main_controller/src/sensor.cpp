#include "sensor.h"
#include "config.h"
#include "logger.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

static Adafruit_MPU6050 mpu;
static float baselineAccelMag = 1.0f; // ~1g เมื่อวางนิ่ง
static float lastAccelDelta = 0.0f;
static float lastGyroDelta = 0.0f;
static int consecutiveMotionHits = 0;
static unsigned long lastVibrationTrigger = 0;

// --------- HC-SR04 ---------
static float readUltrasonicCm() {
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);  delayMicroseconds(2);
    digitalWrite(ULTRASONIC_TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

    long duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH, 30000UL); // timeout 30ms (~5m max)
    if (duration == 0) return -1.0f; // timeout / ไม่มี echo
    return duration * 0.0343f / 2.0f; // speed of sound cm/us / 2 (round trip)
}

bool Sensor::isParcelDetected() {
    int validSamples = 0;
    float sum = 0;
    for (int i = 0; i < ULTRASONIC_SAMPLE_COUNT; i++) {
        float d = readUltrasonicCm();
        if (d > 0) { sum += d; validSamples++; }
        delay(10);
    }
    if (validSamples == 0) return false;
    float avg = sum / validSamples;
    return avg <= ULTRASONIC_THRESHOLD_CM;
}

// --------- MPU6050 + SW-420 Sensor Fusion ---------
void Sensor::begin() {
    pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
    pinMode(ULTRASONIC_ECHO_PIN, INPUT);
    pinMode(SW420_PIN, INPUT);

    Wire.begin(MPU6050_SDA_PIN, MPU6050_SCL_PIN);
    if (!mpu.begin()) {
        Log::error("Sensor", "MPU6050 not found! Check wiring.");
    } else {
        mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
        Log::info("Sensor", "MPU6050 initialized");
    }
}

void Sensor::update() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float accelMag = sqrt(a.acceleration.x * a.acceleration.x +
                           a.acceleration.y * a.acceleration.y +
                           a.acceleration.z * a.acceleration.z) / 9.81f; // convert to g

    lastAccelDelta = fabs(accelMag - baselineAccelMag);

    float gyroMag = sqrt(g.gyro.x * g.gyro.x + g.gyro.y * g.gyro.y + g.gyro.z * g.gyro.z) * (180.0f / PI);
    lastGyroDelta = gyroMag;

    bool vibrationHigh = (digitalRead(SW420_PIN) == HIGH);

    bool motionExceeded = (lastAccelDelta > MOTION_ACCEL_THRESHOLD_G) ||
                           (lastGyroDelta > MOTION_GYRO_THRESHOLD_DPS);

    if (motionExceeded || vibrationHigh) {
        consecutiveMotionHits++;
    } else {
        consecutiveMotionHits = 0;
    }
}

bool Sensor::isTheftDetected() {
    // ต้อง trigger ต่อเนื่องหลาย sample (Hysteresis) เพื่อลด False Alarm จากลม/แมลง
    if (consecutiveMotionHits < MOTION_CONSEC_SAMPLES) return false;

    unsigned long now = millis();
    if (now - lastVibrationTrigger < VIBRATION_DEBOUNCE_MS) return false; // debounce

    lastVibrationTrigger = now;
    Log::warnf("Sensor", "THEFT DETECTED - accelDelta=%.2fg gyroDelta=%.2fdps",
               lastAccelDelta, lastGyroDelta);
    return true;
}

float Sensor::lastAccelDeltaG() { return lastAccelDelta; }
float Sensor::lastGyroDeltaDps() { return lastGyroDelta; }
