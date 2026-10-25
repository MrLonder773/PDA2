// ════════════════════════════════════════════════════════
//  PDA 2 — Imu_Class.cpp
//  Общая часть для ESP32 и sim: кэш снимка, производные
//  (pitch/roll/heading), калибровка магнитометра.
//  Железо — в Imu_Class_esp32.cpp, фейк — в Imu_Class_sim.cpp.
// ════════════════════════════════════════════════════════

#include "Imu_Class.h"
#include "../pda2_log.h"
#include "../PDA2.h"     // PDA.Prefs (только в .cpp, Imu_Class.h остаётся чистым)

#include <math.h>

static const float RAD2DEG = 57.29578f;

bool Imu_Class::ok() const             { return _ok; }
bool Imu_Class::magOk() const          { return _mag_present && _mag_fail < MAG_FAIL_LIMIT; }
bool Imu_Class::magCalibrating() const { return _calibrating; }
bool Imu_Class::magCalibrated() const  { return _mag_calibrated; }

pda2_imu_t Imu_Class::get() const {
    pda2_imu_t s = _snap;
    s.mag_valid = _mag_fresh && magOk();
    return s;
}

// ── Приём данных от драйверов ───────────────────────────

void Imu_Class::_set_accel_gyro(float ax, float ay, float az,
                                float gx, float gy, float gz) {
    _snap.ax = ax; _snap.ay = ay; _snap.az = az;
    _snap.gx = gx; _snap.gy = gy; _snap.gz = gz;
    _snap.valid = true;
}

void Imu_Class::_accel_gyro_failed() {
    _snap.valid = false;            // значения остаются прежними
}

void Imu_Class::_mag_bus_result(bool ok) {
    if (ok) _mag_fail = 0;
    else if (_mag_fail < 255) _mag_fail++;
}

void Imu_Class::_set_mag_raw(int16_t x, int16_t y, int16_t z) {
    int16_t raw[3] = {x, y, z};

    if (_calibrating) {
        for (int i = 0; i < 3; i++) {
            if (raw[i] < _cal_min[i]) _cal_min[i] = raw[i];
            if (raw[i] > _cal_max[i]) _cal_max[i] = raw[i];
        }
        double ux = raw[0] / (double)MAG_LSB_PER_UT;
        double uy = raw[1] / (double)MAG_LSB_PER_UT;
        double uz = raw[2] / (double)MAG_LSB_PER_UT;
        double s = ux * ux + uy * uy + uz * uz;
        double* a = _cal_sum;
        a[0] += 1;       a[1] += ux;      a[2] += uy;      a[3] += uz;
        a[4] += ux * ux; a[5] += uy * uy; a[6] += uz * uz;
        a[7] += ux * uy; a[8] += ux * uz; a[9] += uy * uz;
        a[10] += ux * s; a[11] += uy * s; a[12] += uz * s; a[13] += s;
    }

    _snap.mx = (raw[0] - _mag_off[0]) / MAG_LSB_PER_UT;
    _snap.my = (raw[1] - _mag_off[1]) / MAG_LSB_PER_UT;
    _snap.mz = (raw[2] - _mag_off[2]) / MAG_LSB_PER_UT;
    _mag_fresh = true;
}

// ── Производные ─────────────────────────────────────────

float Imu_Class::pitch() const {
    if (!_ok || !_snap.valid) return NAN;
    return atan2f(_snap.ay, sqrtf(_snap.ax * _snap.ax + _snap.az * _snap.az)) * RAD2DEG;
}

float Imu_Class::roll() const {
    if (!_ok || !_snap.valid) return NAN;
    return atan2f(_snap.ax, sqrtf(_snap.ay * _snap.ay + _snap.az * _snap.az)) * RAD2DEG;
}

float Imu_Class::heading() const {
    if (!_ok || !_snap.valid || !magOk()) return NAN;

    float ax = _snap.ax, ay = _snap.ay, az = _snap.az;
    float mx = _snap.mx, my = _snap.my, mz = _snap.mz;

    // H = m × a (восток в осях устройства)
    float hx = my * az - mz * ay;
    float hy = mz * ax - mx * az;
    float hz = mx * ay - my * ax;
    float hn = sqrtf(hx * hx + hy * hy + hz * hz);
    float an = sqrtf(ax * ax + ay * ay + az * az);
    if (hn < 1e-3f || an < 1e-3f) return NAN;   // поле параллельно g или нет данных
    hx /= hn; hy /= hn; hz /= hn;
    ax /= an; ay /= an; az /= an;

    // M = a × H (север в горизонтали); нужна только его y-компонента
    float m_y = az * hx - ax * hz;

    float deg = atan2f(hy, m_y) * RAD2DEG;      // азимут оси Y устройства
    if (deg < 0.0f) deg += 360.0f;
    return deg;
}

// ── Калибровка магнитометра ─────────────────────────────

bool Imu_Class::magCalibStart() {
    if (!magOk()) {
        PDA_LOGE("imu", "magCalibStart(): magnetometer not available");
        return false;
    }
    for (int i = 0; i < 3; i++) { _cal_min[i] = INT16_MAX; _cal_max[i] = INT16_MIN; }
    for (int i = 0; i < 14; i++) _cal_sum[i] = 0.0;
    _calibrating = true;
    return true;
}

float Imu_Class::magCalibProgress() const {
    if (!_calibrating) return 0.0f;
    int32_t min_span = INT32_MAX, max_span = 0;
    for (int i = 0; i < 3; i++) {
        int32_t span = (int32_t)_cal_max[i] - (int32_t)_cal_min[i];
        if (span < 0) span = 0;                 // данных по оси ещё нет
        if (span < min_span) min_span = span;
        if (span > max_span) max_span = span;
    }
    if (max_span == 0) return 0.0f;

    // Готово, когда (1) размах достаточен и (2) по всем осям он сопоставим:
    // у сферы поля размахи равны, значит центр диапазона найден верно.
    float p_abs   = (float)min_span / (float)MAG_CALIB_MIN_SPAN;
    float p_ratio = (float)min_span / (MAG_CALIB_RATIO * (float)max_span);
    float p = p_abs < p_ratio ? p_abs : p_ratio;
    return p > 1.0f ? 1.0f : p;
}

// Подгон сферы МНК: x²+y²+z² = 2ax + 2by + 2cz + d, центр (a,b,c), r² = d + a²+b²+c².
// Работает и при частичном покрытии сферы (в отличие от (min+max)/2).
static bool _fit_sphere(const double* s, double* center, double* radius) {
    double m[4][5] = {
        {4 * s[4], 4 * s[7], 4 * s[8], 2 * s[1], 2 * s[10]},
        {4 * s[7], 4 * s[5], 4 * s[9], 2 * s[2], 2 * s[11]},
        {4 * s[8], 4 * s[9], 4 * s[6], 2 * s[3], 2 * s[12]},
        {2 * s[1], 2 * s[2], 2 * s[3], s[0],     s[13]},
    };
    for (int col = 0; col < 4; col++) {
        int piv = col;
        for (int r = col + 1; r < 4; r++)
            if (fabs(m[r][col]) > fabs(m[piv][col])) piv = r;
        if (fabs(m[piv][col]) < 1e-9) return false;    // вырожденный случай: мало вращения
        for (int k = 0; k < 5; k++) { double t = m[col][k]; m[col][k] = m[piv][k]; m[piv][k] = t; }
        for (int r = col + 1; r < 4; r++) {
            double f = m[r][col] / m[col][col];
            for (int k = col; k < 5; k++) m[r][k] -= f * m[col][k];
        }
    }
    double p[4];
    for (int r = 3; r >= 0; r--) {
        double v = m[r][4];
        for (int k = r + 1; k < 4; k++) v -= m[r][k] * p[k];
        p[r] = v / m[r][r];
    }
    double r2 = p[3] + p[0] * p[0] + p[1] * p[1] + p[2] * p[2];
    if (r2 <= 0.0) return false;
    center[0] = p[0]; center[1] = p[1]; center[2] = p[2];
    *radius = sqrt(r2);
    return true;
}

bool Imu_Class::magCalibStop() {
    if (!_calibrating) return false;
    bool enough = magCalibProgress() >= 1.0f;
    _calibrating = false;

    if (!enough) {
        PDA_LOGE("imu", "magCalibStop(): not enough rotation, offset unchanged");
        return false;
    }

    double c[3], r = 0.0;
    if (!_fit_sphere(_cal_sum, c, &r) || r < MAG_FIELD_MIN_UT || r > MAG_FIELD_MAX_UT) {
        PDA_LOGE("imu", "magCalibStop(): sphere fit failed (r=%.1f uT), offset unchanged", r);
        return false;
    }
    for (int i = 0; i < 3; i++)
        _mag_off[i] = (int16_t)lround(c[i] * MAG_LSB_PER_UT);
    _save_mag_offset();
    _mag_calibrated = true;
    PDA_LOGI("imu", "mag calibrated: off=(%d,%d,%d) field=%.1f uT",
             _mag_off[0], _mag_off[1], _mag_off[2], r);
    return true;
}

void Imu_Class::magCalibReset() {
    _calibrating = false;
    _mag_calibrated = false;
    _mag_off[0] = _mag_off[1] = _mag_off[2] = 0;
    PDA.Prefs.clearMagOffset();
}

// ── Хранение offset (Prefs) ─────────────────────────────

void Imu_Class::_load_mag_offset() {
    int16_t tmp[3];
    if (PDA.Prefs.getMagOffset(tmp)) {
        for (int i = 0; i < 3; i++) _mag_off[i] = tmp[i];
        _mag_calibrated = true;
        PDA_LOGI("imu", "mag offset loaded: (%d,%d,%d)", _mag_off[0], _mag_off[1], _mag_off[2]);
    } else {
        _mag_off[0] = _mag_off[1] = _mag_off[2] = 0;
        _mag_calibrated = false;
        PDA_LOGI("imu", "no saved mag offset (not calibrated)");
    }
}

void Imu_Class::_save_mag_offset() {
    PDA.Prefs.setMagOffset(_mag_off);
}