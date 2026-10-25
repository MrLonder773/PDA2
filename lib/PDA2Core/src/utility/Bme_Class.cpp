// ════════════════════════════════════════════════════════
//  PDA 2 — Bme_Class.cpp
//  Общая часть для ESP32 и sim: кэш снимка, высота, нуль, QNH.
//  Железо — в Bme_Class_esp32.cpp, фейк — в Bme_Class_sim.cpp.
// ════════════════════════════════════════════════════════

#include "Bme_Class.h"
#include "../pda2_log.h"
#include "../PDA2.h"     // PDA.Prefs (только в .cpp, Bme_Class.h остаётся чистым)

#include <math.h>

static const float ALT_EXP = 0.1903f;      // показатель барометрической формулы

bool Bme_Class::ok() const { return _ok; }

pda2_env_t Bme_Class::get() const { return _snap; }

// ── Приём данных от драйверов ───────────────────────────

void Bme_Class::_set_env(float temp_c, float hum_pct, float press_hpa) {
    if (!(press_hpa >= PRESS_MIN && press_hpa <= PRESS_MAX)) {
        _snap.valid = false;        // неправдоподобное давление — считаем сбоем чтения
        return;
    }
    _snap.temp_c    = temp_c;
    _snap.hum_pct   = hum_pct;
    _snap.press_hpa = press_hpa;
    _snap.valid     = true;
}

void Bme_Class::_env_failed() {
    _snap.valid = false;            // значения остаются прежними
}

// ── Высота ──────────────────────────────────────────────

// h = 44330 · (1 − (p/p0)^0.1903) — международная барометрическая формула
float Bme_Class::_altitude(float p, float p0) {
    if (p <= 0.0f || p0 <= 0.0f) return NAN;
    return 44330.0f * (1.0f - powf(p / p0, ALT_EXP));
}

bool Bme_Class::zeroHere() {
    if (!_snap.valid) {
        PDA_LOGE("bme", "zeroHere(): no valid pressure data");
        return false;
    }
    _zero_hpa = _snap.press_hpa;
    _zero_set = true;
    return true;
}

void Bme_Class::zeroClear()         { _zero_set = false; }
bool Bme_Class::zeroSet() const     { return _zero_set; }

float Bme_Class::altitudeRel() const {
    if (!_snap.valid || !_zero_set) return NAN;
    return _altitude(_snap.press_hpa, _zero_hpa);
}

float Bme_Class::altitudeAbs() const {
    if (!_snap.valid) return NAN;
    return _altitude(_snap.press_hpa, _qnh);
}

// ── QNH (Prefs) ─────────────────────────────────────────

bool Bme_Class::setQnh(float hpa) {
    if (!(hpa >= QNH_MIN && hpa <= QNH_MAX)) {      // NaN тоже отклоняется
        PDA_LOGE("bme", "setQnh(): %.1f hPa out of range, unchanged", hpa);
        return false;
    }
    _qnh = hpa;
    PDA.Prefs.setQnh(hpa);
    return true;
}

// p0 = p / (1 − h/44330)^(1/0.1903) — обратная к _altitude()
bool Bme_Class::setQnhFromAltitude(float meters) {
    if (!_snap.valid) {
        PDA_LOGE("bme", "setQnhFromAltitude(): no valid pressure data");
        return false;
    }
    float k = 1.0f - meters / 44330.0f;
    if (!(k > 0.0f)) {                              // NaN и высота ≥ 44330 м отклоняются
        PDA_LOGE("bme", "setQnhFromAltitude(): %.1f m out of range, unchanged", meters);
        return false;
    }
    float p0 = _snap.press_hpa / powf(k, 1.0f / ALT_EXP);
    return setQnh(p0);                              // проверка диапазона и сохранение — там
}

float Bme_Class::qnh() const { return _qnh; }

void Bme_Class::_load_qnh() {
    float v = PDA.Prefs.getQnh();
    _qnh = (v >= QNH_MIN && v <= QNH_MAX) ? v : QNH_DEFAULT;
    PDA_LOGI("bme", "QNH = %.2f hPa", _qnh);
}