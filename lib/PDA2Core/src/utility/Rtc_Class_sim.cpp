// ════════════════════════════════════════════════════════
//  PDA 2 — Rtc_Class_sim.cpp
//  PC-реализация: системные часы + смещение.
//  Компилируется, только если PDA2_SIM определён —
//  в прошивке ESP32 тело пустое (см. Rtc_Class_esp32.cpp).
//
//  set() не трогает реальные часы ОС (это требует прав
//  администратора и разного кода под Windows/Linux) — вместо
//  этого храним смещение между системным временем и "временем
//  PDA2". get() = системное время + смещение, тикает само,
//  как настоящие часы, но независимо от часов ПК.
// ════════════════════════════════════════════════════════

#include "Rtc_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"

#ifdef PDA2_SIM

#include <ctime>

static time_t _sim_offset_sec = 0;

void Rtc_Class::begin() {
    _ok = true;
    PDA_LOGI("rtc", "SIM: system clock ok");
}

bool Rtc_Class::ok() { return _ok; }

pda2_time_t Rtc_Class::get() {
    pda2_time_t t = {};
    if (!_ok) return t;

    time_t now = time(nullptr) + _sim_offset_sec;
    struct tm* lt = localtime(&now);
    if (!lt) return t;

    t.year    = (uint16_t)(1900 + lt->tm_year);
    t.month   = (uint8_t)(lt->tm_mon + 1);
    t.day     = (uint8_t)lt->tm_mday;
    t.hour    = (uint8_t)lt->tm_hour;
    t.minute  = (uint8_t)lt->tm_min;
    t.second  = (uint8_t)lt->tm_sec;
    // tm_wday: 0=Вс..6=Сб. pda2_time_t: 1=Пн..7=Вс.
    t.weekday = (lt->tm_wday == 0) ? 7 : (uint8_t)lt->tm_wday;

    return t;
}

void Rtc_Class::set(const pda2_time_t& t) {
    if (!_ok) return;

    struct tm target = {};
    target.tm_year = t.year - 1900;
    target.tm_mon  = t.month - 1;
    target.tm_mday = t.day;
    target.tm_hour = t.hour;
    target.tm_min  = t.minute;
    target.tm_sec  = t.second;
    target.tm_isdst = -1;

    time_t target_epoch = mktime(&target);
    time_t now_epoch    = time(nullptr);
    _sim_offset_sec = target_epoch - now_epoch;

    PDA_LOGI("rtc", "SIM: time set: %04d-%02d-%02d %02d:%02d:%02d",
             t.year, t.month, t.day, t.hour, t.minute, t.second);
}

#endif // PDA2_SIM