#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — Imu_Class.h
//  BMI160, raw I2C. Без внешних библиотек.
// ════════════════════════════════════════════════════════

#include <stdint.h>

struct pda2_imu_t {
    float ax, ay, az;   // акселерометр (g)
    float gx, gy, gz;   // гироскоп (°/s)
};

class Imu_Class {
public:
    void begin();
    bool ok();
    pda2_imu_t get();

private:
    bool _ok = false;
};