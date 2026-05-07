// ════════════════════════════════════════════════════════
//  PDA 2 — Fs_Class.cpp
// ════════════════════════════════════════════════════════

#include "Fs_Class.h"
#include "../pda2_log.h"
#include <SPIFFS.h>

void Fs_Class::begin() {
    _ok = SPIFFS.begin(false);  // [A-20] false — не форматировать при ошибке
    if (_ok) {
        PDA_LOGI("fs", "SPIFFS ok. Total: %u  Used: %u",
                 SPIFFS.totalBytes(), SPIFFS.usedBytes());
    } else {
        PDA_LOGE("fs", "SPIFFS mount failed");
    }
}

bool Fs_Class::ok() {
    return _ok;
}

size_t Fs_Class::totalBytes() {
    return _ok ? SPIFFS.totalBytes() : 0;
}

size_t Fs_Class::usedBytes() {
    return _ok ? SPIFFS.usedBytes() : 0;
}

size_t Fs_Class::freeBytes() {
    return _ok ? (SPIFFS.totalBytes() - SPIFFS.usedBytes()) : 0;
}