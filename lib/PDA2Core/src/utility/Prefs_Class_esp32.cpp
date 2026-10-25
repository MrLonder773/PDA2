// ════════════════════════════════════════════════════════
//  PDA 2 — Prefs_Class_esp32.cpp
//  NVS-обёртка поверх Arduino Preferences (ESP-IDF NVS).
//
//  Компилируется, только если PDA2_SIM НЕ определён —
//  в сборке симулятора своя реализация, файл-бэкенд
//  (см. Prefs_Class_sim.cpp).
// ════════════════════════════════════════════════════════

#include "Prefs_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"

#ifndef PDA2_SIM

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

bool Prefs_Class::getWifi() {
    return _prefs.getBool("wifi", true);
}

void Prefs_Class::setWifi(bool val) {
    _prefs.putBool("wifi", val);
}

PdaStr32 Prefs_Class::getWifiSSID() { return PdaStr32(_prefs.getString("wifi_ssid", "").c_str()); }
void     Prefs_Class::setWifiSSID(const PdaStr32& ssid) { _prefs.putString("wifi_ssid", ssid.c_str()); }
PdaStr32 Prefs_Class::getWifiPass() { return PdaStr32(_prefs.getString("wifi_pass", "").c_str()); }
void     Prefs_Class::setWifiPass(const PdaStr32& pass) { _prefs.putString("wifi_pass", pass.c_str()); }
PdaStr32 Prefs_Class::getApSSID()   { return PdaStr32(_prefs.getString("ap_ssid", "").c_str()); }
void     Prefs_Class::setApSSID(const PdaStr32& ssid)   { _prefs.putString("ap_ssid", ssid.c_str()); }
PdaStr32 Prefs_Class::getApPass()   { return PdaStr32(_prefs.getString("ap_pass", "").c_str()); }
void     Prefs_Class::setApPass(const PdaStr32& pass)   { _prefs.putString("ap_pass", pass.c_str()); }

bool     Prefs_Class::getBtEnabled()                  { return _prefs.getBool("bt_en", false); }
void     Prefs_Class::setBtEnabled(bool v)            { _prefs.putBool("bt_en", v); }
PdaStr32 Prefs_Class::getBtLastDevice()               { return PdaStr32(_prefs.getString("bt_last", "").c_str()); }
void     Prefs_Class::setBtLastDevice(const PdaStr32& m) { _prefs.putString("bt_last", m.c_str()); }
uint8_t  Prefs_Class::getBtLastDeviceType()           { return _prefs.getUChar("bt_last_t", 1); }
void     Prefs_Class::setBtLastDeviceType(uint8_t t)  { _prefs.putUChar("bt_last_t", t); }
PdaStr32 Prefs_Class::getBtLastDeviceName()               { return PdaStr32(_prefs.getString("bt_last_n", "").c_str()); }
void     Prefs_Class::setBtLastDeviceName(const PdaStr32& n){ _prefs.putString("bt_last_n", n.c_str()); }

bool Prefs_Class::getMagOffset(int16_t out[3]) {
    if (!_prefs.isKey("mag_off")) return false;
    if (_prefs.getBytesLength("mag_off") != sizeof(int16_t) * 3) return false;
    int16_t tmp[3];
    if (_prefs.getBytes("mag_off", tmp, sizeof(tmp)) != sizeof(tmp)) return false;
    out[0] = tmp[0]; out[1] = tmp[1]; out[2] = tmp[2];
    return true;
}
void Prefs_Class::setMagOffset(const int16_t in[3]) { _prefs.putBytes("mag_off", in, sizeof(int16_t) * 3); }
void Prefs_Class::clearMagOffset()                  { _prefs.remove("mag_off"); }

float Prefs_Class::getQnh()           { return _prefs.getFloat("qnh", 1013.25f); }
void  Prefs_Class::setQnh(float hpa)  { _prefs.putFloat("qnh", hpa); }

int16_t Prefs_Class::getTzOffset()            { return _prefs.getShort("tz_off", 120); }
void    Prefs_Class::setTzOffset(int16_t min) { _prefs.putShort("tz_off", min); }
uint8_t Prefs_Class::getDstRule()             { return _prefs.getUChar("dst_rule", 1); }
void    Prefs_Class::setDstRule(uint8_t rule) { _prefs.putUChar("dst_rule", rule); }

#endif // !PDA2_SIM