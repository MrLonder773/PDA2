// ════════════════════════════════════════════════════════
//  PDA 2 — Rtc_Class_esp32.cpp
//  DS3231 raw I2C. Регистры: 0x00–0x06 (время), 0x0F (статус, бит7 = OSF).
//  Только железо: сырое стандартное время. Пояс и переход на летнее —
//  в Rtc_Class.cpp.
//  Компилируется, только если PDA2_SIM НЕ определён —
//  в сборке симулятора тело пустое (см. Rtc_Class_sim.cpp).
// ════════════════════════════════════════════════════════

#include "Rtc_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"

#ifndef PDA2_SIM

#include <Wire.h>

static const uint8_t REG_HOURS  = 0x02;
static const uint8_t REG_STATUS = 0x0F;
static const uint8_t OSF_BIT    = 0x80;   // Oscillator Stop Flag
static const uint8_t HRS_12H    = 0x40;   // бит6 часового регистра: 12-часовой режим
static const uint8_t HRS_PM     = 0x20;   // бит5 в 12h-режиме: PM

static inline uint8_t bcd2dec(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }
static inline uint8_t dec2bcd(uint8_t d) { return ((d / 10) << 4) | (d % 10); }

static bool _read_reg(uint8_t reg, uint8_t& out) {
    Wire.beginTransmission(PDA2_I2C_RTC);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((uint8_t)PDA2_I2C_RTC, (uint8_t)1) != 1) return false;
    out = Wire.read();
    return true;
}

static bool _write_reg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(PDA2_I2C_RTC);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

bool Rtc_Class::_hw_begin() {
    Wire.beginTransmission(PDA2_I2C_RTC);
    uint8_t err = Wire.endTransmission();
    if (err != 0) {
        PDA_LOGE("rtc", "DS3231 not found (err=%d)", err);
        return false;
    }

    // Убедиться, что 24h режим. Если чип в 12h — перевести час в 24h-значение
    // (простой сброс бита6 оставил бы бит5=PM, который читается как «+20 часов»).
    uint8_t hrs;
    if (_read_reg(REG_HOURS, hrs)) {
        if (hrs & HRS_12H) {
            uint8_t h12 = bcd2dec(hrs & 0x1F);                    // 1..12
            uint8_t h24 = (h12 % 12) + ((hrs & HRS_PM) ? 12 : 0);
            if (!_write_reg(REG_HOURS, dec2bcd(h24)))             // bit6=0, bit5 — часть BCD
                PDA_LOGE("rtc", "failed to switch to 24h mode");
        }
    } else {
        PDA_LOGE("rtc", "failed to read hours register");
    }

    uint8_t st;
    if (_read_reg(REG_STATUS, st) && (st & OSF_BIT))
        PDA_LOGI("rtc", "DS3231 ok, but clock not set (OSF)");
    else
        PDA_LOGI("rtc", "DS3231 ok");
    return true;
}

bool Rtc_Class::_hw_read(pda2_time_t& raw, bool& osc_stopped) const {
    // Один запрос: 0x00..0x0F (время + будильники + control + status)
    Wire.beginTransmission(PDA2_I2C_RTC);
    Wire.write(0x00);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((uint8_t)PDA2_I2C_RTC, (uint8_t)16) != 16) return false;

    uint8_t r[16];
    for (uint8_t i = 0; i < 16; i++) r[i] = Wire.read();

    raw = {};
    raw.second = bcd2dec(r[0] & 0x7F);
    raw.minute = bcd2dec(r[1] & 0x7F);
    raw.hour   = bcd2dec(r[2] & 0x3F);
    raw.day    = bcd2dec(r[4] & 0x3F);          // регистр дня недели r[3] не используем
    raw.month  = bcd2dec(r[5] & 0x1F);
    raw.year   = 2000 + bcd2dec(r[6]);
    osc_stopped = (r[REG_STATUS] & OSF_BIT) != 0;
    return true;
}

bool Rtc_Class::_hw_write(const pda2_time_t& raw) {
    Wire.beginTransmission(PDA2_I2C_RTC);
    Wire.write(0x00);
    Wire.write(dec2bcd(raw.second));
    Wire.write(dec2bcd(raw.minute));
    Wire.write(dec2bcd(raw.hour));          // 24h, bit6=0
    Wire.write(_weekday_of(raw.year, raw.month, raw.day));
    Wire.write(dec2bcd(raw.day));
    Wire.write(dec2bcd(raw.month));
    Wire.write(dec2bcd((uint8_t)(raw.year - 2000)));
    if (Wire.endTransmission() != 0) {
        PDA_LOGE("rtc", "write failed (I2C)");
        return false;
    }

    // Время заведено — сбросить флаг остановки осциллятора
    uint8_t st;
    if (!_read_reg(REG_STATUS, st) || !_write_reg(REG_STATUS, st & ~OSF_BIT)) {
        PDA_LOGE("rtc", "failed to clear OSF");
        return false;
    }
    return true;
}

#endif // !PDA2_SIM