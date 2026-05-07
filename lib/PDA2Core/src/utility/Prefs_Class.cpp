// ════════════════════════════════════════════════════════
//  PDA 2 — Prefs_Class.cpp
// ════════════════════════════════════════════════════════

#include "Prefs_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"
#include <Preferences.h>

static Preferences _prefs;

void Prefs_Class::begin() {
    _prefs.begin(PDA2_NVS_NS, false);
    PDA_LOGI("prefs", "NVS opened, namespace: %s", PDA2_NVS_NS);
}

uint8_t Prefs_Class::getBrightness() {
    return _prefs.getUChar("brightness", 200);
}

void Prefs_Class::setBrightness(uint8_t v) {
    _prefs.putUChar("brightness", v);
}

uint8_t Prefs_Class::getRotation() {
    return _prefs.getUChar("rotation", PDA2_ROTATION);
}

void Prefs_Class::setRotation(uint8_t r) {
    _prefs.putUChar("rotation", r);
}

uint8_t Prefs_Class::getTheme() {
    return _prefs.getUChar("theme", PDA2_THEME_DARK);
}

void Prefs_Class::setTheme(uint8_t t) {
    _prefs.putUChar("theme", t);
}

uint32_t Prefs_Class::getSleepTimeout() {
    return _prefs.getUInt("sleep_ms", PDA2_SLEEP_TIMEOUT_MS);
}

void Prefs_Class::setSleepTimeout(uint32_t ms) {
    _prefs.putUInt("sleep_ms", ms);
}

int8_t Prefs_Class::getLastApp() {
    return _prefs.getChar("last_app", -1);
}

void Prefs_Class::setLastApp(int8_t id) {
    _prefs.putChar("last_app", id);
}