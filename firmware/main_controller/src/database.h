#pragma once
// Local tracking-number database stored in ESP32 NVS (no cloud).

#include <Arduino.h>

enum class VerifyResult {
    Valid,
    Unknown,
    AlreadyUsed
};

class LocalDatabase {
public:
    void begin();
    VerifyResult verify(const String& tracking) const;
    bool markUsed(const String& tracking);
    void resetAllUsed();  // demo helper: clear used flags in NVS

private:
    static constexpr size_t kMaxEntries = 16;
    struct Entry {
        const char* tracking;
        bool used;
    };

    Entry entries_[kMaxEntries];
    size_t count_ = 0;

    int findIndex(const String& tracking) const;
    void loadUsedFlags();
    void saveUsedFlag(size_t index, bool used);
};

extern LocalDatabase localDatabase;
