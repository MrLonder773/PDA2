// ════════════════════════════════════════════════════════
//  PDA 2 — Imu_Class_sim.cpp
//  PC-реализация: фейковые данные покоя (лежит экраном вверх)
//  + небольшой шум. Магнитометр: поле медленно вращается
//  (heading() крутится ~15°/с) и имеет постоянное смещение
//  hard-iron. Во время калибровки поле «кувыркается» по всей
//  сфере, чтобы калибровку можно было пройти без платы.
//  Компилируется, только если PDA2_SIM определён —
//  в прошивке ESP32 тело пустое (см. Imu_Class_esp32.cpp).
// ════════════════════════════════════════════════════════

#include "Imu_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"

#ifdef PDA2_SIM

#include <cstdlib>
#include <ctime>
#include <cmath>
#include <chrono>

static bool _seeded = false;

// Случайное число в диапазоне [-range; +range]
static float _jitter(float range) {
    return ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * range;
}

static float _t_sec() {
    static const auto t0 = std::chrono::steady_clock::now();
    return std::chrono::duration<float>(std::chrono::steady_clock::now() - t0).count();
}

void Imu_Class::begin() {
    if (!_seeded) {
        srand((unsigned)time(nullptr));
        _seeded = true;
    }
    _ok = true;
    _mag_present = true;
    _mag_fail = 0;
    _load_mag_offset();
    update();
    PDA_LOGI("imu", "SIM: fake IMU+mag ok (resting, az~1g)");
}

void Imu_Class::update() {
    // Покой, экран вверх: почти вся гравитация на Z, лёгкий шум на X/Y.
    _set_accel_gyro(_jitter(0.03f), _jitter(0.03f), 1.0f + _jitter(0.02f),
                    _jitter(1.5f),  _jitter(1.5f),  _jitter(1.5f));

    // Магнитометр: |B| ≈ 49 µT (горизонталь 20, вертикаль вниз 45) + hard-iron.
    const float B_H = 20.0f, B_V = 45.0f;
    const float bias[3] = {12.0f, -7.0f, 20.0f};
    const float t = _t_sec();
    float v[3];

    if (_calibrating) {
        float B = sqrtf(B_H * B_H + B_V * B_V);
        float a = 0.6f * t, b = 1.7f * t;
        v[0] = B * sinf(a) * cosf(b);
        v[1] = B * sinf(a) * sinf(b);
        v[2] = B * cosf(a);
    } else {
        float psi = 15.0f * t * 0.01745329f;    // азимут верха экрана
        v[0] = -B_H * sinf(psi);
        v[1] =  B_H * cosf(psi);
        v[2] = -B_V;
    }

    int16_t raw[3];
    for (int i = 0; i < 3; i++)
        raw[i] = (int16_t)lroundf((v[i] + bias[i] + _jitter(0.3f)) * MAG_LSB_PER_UT);

    _mag_bus_result(true);
    _set_mag_raw(raw[0], raw[1], raw[2]);
}

#endif // PDA2_SIM