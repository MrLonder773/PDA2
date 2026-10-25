// ════════════════════════════════════════════════════════
//  PDA 2 — Rtc_Class_sim.cpp
//  PC-реализация «чипа»: UTC системных часов + смещение. Сырое
//  время = стандартное (UTC + tz_off), как в DS3231, поэтому в sim
//  часовой пояс берётся из Prefs, а не из пояса ПК.
//  Переход на летнее время — в Rtc_Class.cpp.
//  Компилируется, только если PDA2_SIM определён —
//  в прошивке ESP32 тело пустое (см. Rtc_Class_esp32.cpp).
//
//  _hw_write() не трогает часы ОС: хранится смещение между
//  системным временем и «временем чипа». Часы тикают сами.
// ════════════════════════════════════════════════════════

#include "Rtc_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"

#ifdef PDA2_SIM

#include <ctime>

static int64_t _sim_offset_sec = 0;

bool Rtc_Class::_hw_begin() {
    _sim_offset_sec = (int64_t)_tz_off * 60;     // «чип» стартует со стандартным временем пояса
    PDA_LOGI("rtc", "SIM: system clock ok");
    return true;
}

bool Rtc_Class::_hw_read(pda2_time_t& raw, bool& osc_stopped) const {
    int64_t now = (int64_t)time(nullptr) + _sim_offset_sec;
    raw = {};
    _from_epoch(now, raw);
    raw.weekday = 0;                             // как у реального _hw_read: считает общий код
    osc_stopped = false;
    return true;
}

bool Rtc_Class::_hw_write(const pda2_time_t& raw) {
    _sim_offset_sec = _to_epoch(raw) - (int64_t)time(nullptr);
    return true;
}

#endif // PDA2_SIM