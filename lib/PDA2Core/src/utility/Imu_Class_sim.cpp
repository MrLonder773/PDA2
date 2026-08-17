// ════════════════════════════════════════════════════════
//  PDA 2 — Imu_Class_sim.cpp
//  PC-реализация: фейковые данные покоя (лежит экраном вверх)
//  + небольшой случайный шум по всем осям.
//  Компилируется, только если PDA2_SIM определён —
//  в прошивке ESP32 тело пустое (см. Imu_Class_esp32.cpp).
// ════════════════════════════════════════════════════════

#include "Imu_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"

#ifdef PDA2_SIM

#include <cstdlib>
#include <ctime>

static bool _seeded = false;

// Случайное число в диапазоне [-range; +range]
static float _jitter(float range) {
    return ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * range;
}

void Imu_Class::begin() {
    if (!_seeded) {
        srand((unsigned)time(nullptr));
        _seeded = true;
    }
    _ok = true;
    PDA_LOGI("imu", "SIM: fake IMU ok (resting, az~1g)");
}

bool Imu_Class::ok() { return _ok; }

pda2_imu_t Imu_Class::get() {
    pda2_imu_t d = {};
    if (!_ok) return d;

    // Покой, экран вверх: почти вся гравитация на Z, лёгкий шум на X/Y.
    d.ax = _jitter(0.03f);
    d.ay = _jitter(0.03f);
    d.az = 1.0f + _jitter(0.02f);

    // Гироскоп — почти неподвижно, лёгкий дрожащий шум.
    d.gx = _jitter(1.5f);
    d.gy = _jitter(1.5f);
    d.gz = _jitter(1.5f);

    return d;
}

#endif // PDA2_SIM