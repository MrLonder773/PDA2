// ════════════════════════════════════════════════════════
//  PDA 2 — Bme_Class_sim.cpp
//  PC-реализация: фейковые данные комнаты. Температура и влажность
//  медленно дрейфуют, давление колеблется ±0.6 гПа с периодом ~31 с
//  (это ±5 м высоты), чтобы altitudeRel() было заметно после zeroHere().
//  Компилируется, только если PDA2_SIM определён —
//  в прошивке ESP32 тело пустое (см. Bme_Class_esp32.cpp).
// ════════════════════════════════════════════════════════

#include "Bme_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"

#ifdef PDA2_SIM

#include <cmath>
#include <chrono>

static float _t_sec() {
    static const auto t0 = std::chrono::steady_clock::now();
    return std::chrono::duration<float>(std::chrono::steady_clock::now() - t0).count();
}

void Bme_Class::begin() {
    _ok = true;
    _load_qnh();
    _polled = false;
    update();
    PDA_LOGI("bme", "SIM: fake BME280 ok");
}

void Bme_Class::update() {
    if (!_ok) return;

    uint32_t now = (uint32_t)(_t_sec() * 1000.0f);
    if (_polled && (uint32_t)(now - _last_ms) < UPDATE_PERIOD_MS) return;
    _polled = true;
    _last_ms = now;

    const float t = _t_sec();
    _set_env(22.0f + 1.5f * sinf(0.05f * t),        // °C
             45.0f + 5.0f * sinf(0.03f * t),        // %RH
             1005.0f + 0.6f * sinf(0.2f * t));      // гПа
}

#endif // PDA2_SIM