// ════════════════════════════════════════════════════════
//  PDA 2 — Wifi_Class_esp32.cpp
//  Компилируется, только если PDA2_SIM НЕ определён.
// ════════════════════════════════════════════════════════

#include "Wifi_Class.h"

#ifndef PDA2_SIM

#include <WiFi.h>
#include <PDA2.h>
#include "../pda2_log.h"

static WifiState _state              = WifiState::OFF;
static uint32_t  _connect_started_ms = 0;
static uint32_t  _fail_at_ms         = 0;

static constexpr uint32_t STA_CONNECT_TIMEOUT_MS = 15000;
static constexpr uint32_t STA_RETRY_INTERVAL_MS  = 10000;

static bool      _no_ssid            = false;   // последняя попытка — пустой SSID: не повторять

void Wifi_Class::begin() {
    if (PDA.Prefs.getWifi()) {
        connectSTA(PDA.Prefs.getWifiSSID(), PDA.Prefs.getWifiPass());
    }
}

void Wifi_Class::update() {
    switch (_state) {
        case WifiState::STA_CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                _state = WifiState::STA_CONNECTED;
                PDA_LOGI("wifi", "STA connected, IP=%s", WiFi.localIP().toString().c_str());
            } else if (millis() - _connect_started_ms > STA_CONNECT_TIMEOUT_MS) {
                _state = WifiState::STA_FAILED;
                _fail_at_ms = millis();
                PDA_LOGW("wifi", "STA connect timeout");
            }
            break;

        case WifiState::STA_FAILED:
            if (_no_ssid) break;   // SSID не задан — повторять нечего, ждём setEnabled()/connectSTA()
            if (millis() - _fail_at_ms > STA_RETRY_INTERVAL_MS) {
                PDA_LOGI("wifi", "STA retry");
                connectSTA(PDA.Prefs.getWifiSSID(), PDA.Prefs.getWifiPass());
            }
            break;

        default:
            break;
    }
}

void Wifi_Class::setEnabled(bool on) {
    if (on) connectSTA(PDA.Prefs.getWifiSSID(), PDA.Prefs.getWifiPass());
    else    disconnectSTA();
}

void Wifi_Class::connectSTA(const PdaStr32& ssid, const PdaStr32& pass) {
    if (ssid.c_str()[0] == '\0') {
        PDA_LOGW("wifi", "connectSTA: empty SSID, skipped");
        _no_ssid = true;
        _state = WifiState::STA_FAILED;
        _fail_at_ms = millis();
        return;
    }
    _no_ssid = false;
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    _state = WifiState::STA_CONNECTING;
    _connect_started_ms = millis();
    PDA_LOGI("wifi", "STA connecting to %s", ssid.c_str());
}

void Wifi_Class::disconnectSTA() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    _state = WifiState::OFF;
    PDA_LOGI("wifi", "STA off");
}

void Wifi_Class::startAP(const PdaStr32& ssid, const PdaStr32& pass) {
    WiFi.mode(WIFI_AP);
    bool ok = pass.c_str()[0] != '\0'
        ? WiFi.softAP(ssid.c_str(), pass.c_str())
        : WiFi.softAP(ssid.c_str());
    _state = ok ? WifiState::AP_ACTIVE : WifiState::OFF;
    PDA_LOGI("wifi", "AP %s (%s)", ok ? "started" : "failed", ssid.c_str());
}

void Wifi_Class::stopAP() {
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    _state = WifiState::OFF;
    PDA_LOGI("wifi", "AP stopped");
}

WifiState Wifi_Class::state()       { return _state; }
bool      Wifi_Class::isConnected() { return _state == WifiState::STA_CONNECTED; }
int       Wifi_Class::apClientCount() {
    return (_state == WifiState::AP_ACTIVE) ? (int)WiFi.softAPgetStationNum() : 0;
}

#endif // !PDA2_SIM