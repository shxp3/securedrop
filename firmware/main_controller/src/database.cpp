#include "database.h"
#include <Preferences.h>

LocalDatabase localDatabase;
static Preferences prefs;

void LocalDatabase::begin() {
    // Seed list (compile-time). Used flags persist in NVS across reboots.
    count_ = 0;
    auto add = [this](const char* id) {
        if (count_ < kMaxEntries) {
            entries_[count_].tracking = id;
            entries_[count_].used = false;
            ++count_;
        }
    };

    add("TH1234567890");
    add("TH9988776655");
    add("JT5566778899");

    prefs.begin("securedrop", false);
    loadUsedFlags();
    Serial.printf("[Database] Loaded %u tracking numbers\n",
                  static_cast<unsigned>(count_));
    for (size_t i = 0; i < count_; ++i) {
        Serial.printf("  - %s (%s)\n",
                      entries_[i].tracking,
                      entries_[i].used ? "used" : "available");
    }
}

int LocalDatabase::findIndex(const String& tracking) const {
    for (size_t i = 0; i < count_; ++i) {
        if (tracking.equalsIgnoreCase(entries_[i].tracking)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void LocalDatabase::loadUsedFlags() {
    for (size_t i = 0; i < count_; ++i) {
        const String key = String("u_") + String(i);
        entries_[i].used = prefs.getBool(key.c_str(), false);
    }
}

void LocalDatabase::saveUsedFlag(size_t index, bool used) {
    if (index >= count_) {
        return;
    }
    entries_[index].used = used;
    const String key = String("u_") + String(index);
    prefs.putBool(key.c_str(), used);
}

VerifyResult LocalDatabase::verify(const String& tracking) const {
    const int idx = findIndex(tracking);
    if (idx < 0) {
        return VerifyResult::Unknown;
    }
    if (entries_[idx].used) {
        return VerifyResult::AlreadyUsed;
    }
    return VerifyResult::Valid;
}

bool LocalDatabase::markUsed(const String& tracking) {
    const int idx = findIndex(tracking);
    if (idx < 0) {
        return false;
    }
    saveUsedFlag(static_cast<size_t>(idx), true);
    Serial.printf("[Database] Marked used: %s\n", tracking.c_str());
    return true;
}

void LocalDatabase::resetAllUsed() {
    for (size_t i = 0; i < count_; ++i) {
        saveUsedFlag(i, false);
    }
    Serial.println("[Database] All used flags cleared");
}
