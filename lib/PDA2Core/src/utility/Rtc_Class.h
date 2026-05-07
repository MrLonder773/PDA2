#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — Rtc_Class.h
//  DS3231, raw I2C. Без внешних библиотек.
// ════════════════════════════════════════════════════════

#include <stdint.h>

struct pda2_time_t {
    uint16_t year;      // 2000–2099
    uint8_t  month;     // 1–12
    uint8_t  day;       // 1–31
    uint8_t  hour;      // 0–23
    uint8_t  minute;    // 0–59
    uint8_t  second;    // 0–59
    uint8_t  weekday;   // 1=Пн ... 7=Вс
};

class Rtc_Class {
public:
    void begin();
    bool ok();
    pda2_time_t get();
    void        set(const pda2_time_t& t);

private:
    bool _ok = false;
};