// ════════════════════════════════════════════════════════
//  PDA 2 — Rtc_Class.cpp
//  DS3231 raw I2C. Регистры: 0x00–0x06.
// ════════════════════════════════════════════════════════

#include "Rtc_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"
#include <Wire.h>

static inline uint8_t bcd2dec(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }
static inline uint8_t dec2bcd(uint8_t d) { return ((d / 10) << 4) | (d % 10); }

static uint8_t _read_reg(uint8_t reg) {
    Wire.beginTransmission(PDA2_I2C_RTC);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)PDA2_I2C_RTC, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0;
}

void Rtc_Class::begin() {
    Wire.beginTransmission(PDA2_I2C_RTC);
    uint8_t err = Wire.endTransmission();
    _ok = (err == 0);

    if (_ok) {
        // Убедиться что 24h режим (bit6 часового регистра = 0)
        uint8_t hrs = _read_reg(0x02);
        if (hrs & 0x40) {
            Wire.beginTransmission(PDA2_I2C_RTC);
            Wire.write(0x02);
            Wire.write(hrs & ~0x40);
            Wire.endTransmission();
        }
        PDA_LOGI("rtc", "DS3231 ok");
    } else {
        PDA_LOGE("rtc", "DS3231 not found (err=%d)", err);
    }
}

bool Rtc_Class::ok() { return _ok; }

pda2_time_t Rtc_Class::get() {
    pda2_time_t t = {};
    if (!_ok) return t;

    Wire.beginTransmission(PDA2_I2C_RTC);
    Wire.write(0x00);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)PDA2_I2C_RTC, (uint8_t)7);

    if (Wire.available() >= 7) {
        t.second  = bcd2dec(Wire.read() & 0x7F);
        t.minute  = bcd2dec(Wire.read() & 0x7F);
        t.hour    = bcd2dec(Wire.read() & 0x3F);
        t.weekday = Wire.read() & 0x07;
        t.day     = bcd2dec(Wire.read() & 0x3F);
        uint8_t m = Wire.read();
        t.month   = bcd2dec(m & 0x1F);
        t.year    = 2000 + bcd2dec(Wire.read());
    }
    return t;
}

void Rtc_Class::set(const pda2_time_t& t) {
    if (!_ok) return;

    Wire.beginTransmission(PDA2_I2C_RTC);
    Wire.write(0x00);
    Wire.write(dec2bcd(t.second));
    Wire.write(dec2bcd(t.minute));
    Wire.write(dec2bcd(t.hour));          // 24h, bit6=0
    Wire.write(t.weekday & 0x07);
    Wire.write(dec2bcd(t.day));
    Wire.write(dec2bcd(t.month));
    Wire.write(dec2bcd((uint8_t)(t.year - 2000)));
    Wire.endTransmission();

    PDA_LOGI("rtc", "Time set: %04d-%02d-%02d %02d:%02d:%02d",
             t.year, t.month, t.day, t.hour, t.minute, t.second);
}