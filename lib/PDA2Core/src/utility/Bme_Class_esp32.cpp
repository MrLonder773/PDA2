// ════════════════════════════════════════════════════════
//  PDA 2 — Bme_Class_esp32.cpp
//  BME280: chip_id=0x60 (рег. 0xD0), данные 0xF7..0xFE (P,T,H),
//  калибровка 0x88..0x9F + 0xA1 + 0xE1..0xE7.
//  Компенсация — целочисленная по даташиту Bosch (BST-BME280).
//  Компилируется, только если PDA2_SIM НЕ определён —
//  в сборке симулятора тело пустое (см. Bme_Class_sim.cpp).
// ════════════════════════════════════════════════════════

#include "Bme_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"

#ifndef PDA2_SIM

#include <Arduino.h>
#include <Wire.h>

#define BME_REG_CALIB_A    0x88   // 24 байта: T1..T3, P1..P9
#define BME_REG_CALIB_H1   0xA1
#define BME_REG_CHIP_ID    0xD0
#define BME_REG_RESET      0xE0
#define BME_REG_CALIB_B    0xE1   // 7 байт: H2..H6
#define BME_REG_CTRL_HUM   0xF2
#define BME_REG_CTRL_MEAS  0xF4
#define BME_REG_CONFIG     0xF5
#define BME_REG_DATA       0xF7   // P[3] T[3] H[2]
#define BME_CHIP_ID        0x60
#define BMP_CHIP_ID        0x58   // BMP280 (без влажности) — не поддерживается

// ctrl_meas: osrs_t ×2 (010), osrs_p ×16 (101), mode normal (11)
#define BME_CTRL_MEAS_VAL  0x57
// ctrl_hum: osrs_h ×1
#define BME_CTRL_HUM_VAL   0x01
// config: t_sb 125 мс (010), filter ×4 (010), spi3w off
#define BME_CONFIG_VAL     0x48

static bool _write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

static bool _read_bytes(uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t len) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(addr, len) != len) return false;
    for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
}

// ── Калибровка и компенсация ────────────────────────────
static struct {
    uint16_t T1; int16_t T2, T3;
    uint16_t P1; int16_t P2, P3, P4, P5, P6, P7, P8, P9;
    uint8_t  H1; int16_t H2; uint8_t H3; int16_t H4, H5; int8_t H6;
} _c;

static int32_t _t_fine = 0;

static uint16_t _u16(const uint8_t* p) { return (uint16_t)(p[1] << 8 | p[0]); }
static int16_t  _s16(const uint8_t* p) { return (int16_t)(p[1] << 8 | p[0]); }

static bool _read_calib() {
    uint8_t a[24], h1 = 0, b[7];
    if (!_read_bytes(PDA2_I2C_BME280, BME_REG_CALIB_A,  a, 24)) return false;
    if (!_read_bytes(PDA2_I2C_BME280, BME_REG_CALIB_H1, &h1, 1)) return false;
    if (!_read_bytes(PDA2_I2C_BME280, BME_REG_CALIB_B,  b, 7))  return false;

    _c.T1 = _u16(a + 0);  _c.T2 = _s16(a + 2);  _c.T3 = _s16(a + 4);
    _c.P1 = _u16(a + 6);  _c.P2 = _s16(a + 8);  _c.P3 = _s16(a + 10);
    _c.P4 = _s16(a + 12); _c.P5 = _s16(a + 14); _c.P6 = _s16(a + 16);
    _c.P7 = _s16(a + 18); _c.P8 = _s16(a + 20); _c.P9 = _s16(a + 22);

    _c.H1 = h1;
    _c.H2 = _s16(b + 0);
    _c.H3 = b[2];
    _c.H4 = (int16_t)(((int16_t)(int8_t)b[3]) * 16 + (b[4] & 0x0F));
    _c.H5 = (int16_t)(((int16_t)(int8_t)b[5]) * 16 + (b[4] >> 4));
    _c.H6 = (int8_t)b[6];
    return true;
}

// °C; заодно обновляет _t_fine (нужен для давления и влажности)
static float _comp_temp(int32_t adc_T) {
    int32_t v1 = ((((adc_T >> 3) - ((int32_t)_c.T1 << 1))) * ((int32_t)_c.T2)) >> 11;
    int32_t d  = (adc_T >> 4) - (int32_t)_c.T1;
    int32_t v2 = (((d * d) >> 12) * ((int32_t)_c.T3)) >> 14;
    _t_fine = v1 + v2;
    return ((_t_fine * 5 + 128) >> 8) / 100.0f;
}

// гПа
static float _comp_press(int32_t adc_P) {
    int64_t v1 = (int64_t)_t_fine - 128000;
    int64_t v2 = v1 * v1 * (int64_t)_c.P6;
    v2 += (v1 * (int64_t)_c.P5) << 17;
    v2 += ((int64_t)_c.P4) << 35;
    v1 = ((v1 * v1 * (int64_t)_c.P3) >> 8) + ((v1 * (int64_t)_c.P2) << 12);
    v1 = ((((int64_t)1 << 47) + v1) * (int64_t)_c.P1) >> 33;
    if (v1 == 0) return 0.0f;                       // защита от деления на ноль
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - v2) * 3125) / v1;
    v1 = ((int64_t)_c.P9 * (p >> 13) * (p >> 13)) >> 25;
    v2 = ((int64_t)_c.P8 * p) >> 19;
    p = ((p + v1 + v2) >> 8) + ((int64_t)_c.P7 << 4);
    return (float)p / 256.0f / 100.0f;              // Q24.8 Па → гПа
}

// %RH
static float _comp_hum(int32_t adc_H) {
    int32_t v = _t_fine - 76800;
    v = (((((adc_H << 14) - (((int32_t)_c.H4) << 20) - (((int32_t)_c.H5) * v)) + 16384) >> 15) *
         (((((((v * ((int32_t)_c.H6)) >> 10) * (((v * ((int32_t)_c.H3)) >> 11) + 32768)) >> 10) + 2097152) *
           ((int32_t)_c.H2) + 8192) >> 14));
    v = v - (((((v >> 15) * (v >> 15)) >> 7) * ((int32_t)_c.H1)) >> 4);
    if (v < 0) v = 0;
    if (v > 419430400) v = 419430400;
    return (uint32_t)(v >> 12) / 1024.0f;           // Q22.10 → %RH
}

static bool _bme_begin() {
    uint8_t id = 0;
    bool got = _read_bytes(PDA2_I2C_BME280, BME_REG_CHIP_ID, &id, 1);
    if (got && id == BMP_CHIP_ID) {
        PDA_LOGE("bme", "BMP280 found (id=0x58): no humidity, not supported");
        return false;
    }
    if (!got || id != BME_CHIP_ID) {
        PDA_LOGE("bme", "BME280 not found (id=0x%02X)", id);
        return false;
    }

    bool w = _write_reg(PDA2_I2C_BME280, BME_REG_RESET, 0xB6);   // softreset
    delay(10);
    if (!w || !_read_calib()) {
        PDA_LOGE("bme", "BME280 init failed (I2C error reading calibration)");
        return false;
    }

    // ctrl_hum применяется только после записи ctrl_meas; config — до перехода в normal
    w  = _write_reg(PDA2_I2C_BME280, BME_REG_CTRL_HUM,  BME_CTRL_HUM_VAL);
    w &= _write_reg(PDA2_I2C_BME280, BME_REG_CONFIG,    BME_CONFIG_VAL);
    w &= _write_reg(PDA2_I2C_BME280, BME_REG_CTRL_MEAS, BME_CTRL_MEAS_VAL);
    delay(10);
    if (!w) {
        PDA_LOGE("bme", "BME280 config failed (I2C write error)");
        return false;
    }
    PDA_LOGI("bme", "BME280 ok (T x2, P x16, H x1, IIR x4)");
    return true;
}

void Bme_Class::begin() {
    _ok = _bme_begin();
    _load_qnh();
    _polled = false;
    update();       // первый снимок сразу, чтобы get() не отдавал пустоту
}

void Bme_Class::update() {
    if (!_ok) return;

    uint32_t now = millis();
    if (_polled && (uint32_t)(now - _last_ms) < UPDATE_PERIOD_MS) return;
    _polled = true;
    _last_ms = now;

    uint8_t d[8];
    if (!_read_bytes(PDA2_I2C_BME280, BME_REG_DATA, d, 8)) {
        _env_failed();
        return;
    }

    int32_t adc_P = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);
    int32_t adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
    int32_t adc_H = ((int32_t)d[6] << 8) | d[7];

    // 0x80000 / 0x8000 — измерение ещё не выполнено (пропущено)
    if (adc_T == 0x80000 || adc_P == 0x80000 || adc_H == 0x8000) {
        _env_failed();
        return;
    }

    float t = _comp_temp(adc_T);        // первым: считает _t_fine
    float p = _comp_press(adc_P);
    float h = _comp_hum(adc_H);
    _set_env(t, h, p);                  // неправдоподобное давление отсеется там
}

#endif // !PDA2_SIM