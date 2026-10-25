// ════════════════════════════════════════════════════════
//  PDA 2 — Imu_Class_esp32.cpp
//  BMI160: chip_id=0xD1, gyro 0x0C, accel 0x12.
//  QMC5883L: chip_id=0xFF (рег. 0x0D), данные 0x00..0x05, статус 0x06.
//  Компилируется, только если PDA2_SIM НЕ определён —
//  в сборке симулятора тело пустое (см. Imu_Class_sim.cpp).
// ════════════════════════════════════════════════════════

#include "Imu_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"

#ifndef PDA2_SIM

#include <Arduino.h>
#include <Wire.h>

// ── BMI160 ──────────────────────────────────────────────
#define BMI160_REG_CHIP_ID    0x00
#define BMI160_REG_GYRO_DATA  0x0C
#define BMI160_REG_ACCEL_DATA 0x12
#define BMI160_REG_ACC_CONF   0x40
#define BMI160_REG_ACC_RANGE  0x41
#define BMI160_REG_GYR_CONF   0x42
#define BMI160_REG_GYR_RANGE  0x43
#define BMI160_REG_CMD        0x7E
#define BMI160_CHIP_ID        0xD1

// Диапазон: акселерометр ±2g, гироскоп ±250°/s
#define ACCEL_SCALE  (2.0f / 32768.0f)
#define GYRO_SCALE   (250.0f / 32768.0f)

// ── QMC5883L ────────────────────────────────────────────
#define QMC_REG_DATA     0x00   // X_L X_H Y_L Y_H Z_L Z_H, затем статус 0x06
#define QMC_REG_CTRL1    0x09
#define QMC_REG_CTRL2    0x0A
#define QMC_REG_PERIOD   0x0B
#define QMC_REG_CHIP_ID  0x0D
#define QMC_CHIP_ID      0xFF
#define QMC_STATUS_DRDY  0x01
#define QMC_STATUS_OVL   0x02

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

static bool _bmi_begin() {
    uint8_t id = 0;
    if (!_read_bytes(PDA2_I2C_IMU, BMI160_REG_CHIP_ID, &id, 1) || id != BMI160_CHIP_ID) {
        PDA_LOGE("imu", "BMI160 not found (id=0x%02X)", id);
        return false;
    }

    bool w = true;
    w &= _write_reg(PDA2_I2C_IMU, BMI160_REG_CMD, 0xB6);   // softreset
    delay(100);
    w &= _write_reg(PDA2_I2C_IMU, BMI160_REG_CMD, 0x11);   // accel normal mode
    delay(100);
    w &= _write_reg(PDA2_I2C_IMU, BMI160_REG_CMD, 0x15);   // gyro normal mode
    delay(100);

    // Явно: диапазоны, ODR и фильтр (не полагаться на дефолты после reset)
    w &= _write_reg(PDA2_I2C_IMU, BMI160_REG_ACC_RANGE, 0x03);  // ±2g
    w &= _write_reg(PDA2_I2C_IMU, BMI160_REG_GYR_RANGE, 0x03);  // ±250°/s
    w &= _write_reg(PDA2_I2C_IMU, BMI160_REG_ACC_CONF,  0x28);  // 100 Гц, normal filter
    w &= _write_reg(PDA2_I2C_IMU, BMI160_REG_GYR_CONF,  0x28);  // 100 Гц, normal filter
    delay(10);

    if (!w) {
        PDA_LOGE("imu", "BMI160 config failed (I2C write error)");
        return false;
    }
    PDA_LOGI("imu", "BMI160 ok (accel=±2g, gyro=±250dps, 100Hz)");
    return true;
}

static bool _mag_begin() {
    uint8_t id = 0;
    if (!_read_bytes(PDA2_I2C_COMPASS, QMC_REG_CHIP_ID, &id, 1) || id != QMC_CHIP_ID)
        return false;                                       // необязателен — не ошибка

    bool w = true;
    w &= _write_reg(PDA2_I2C_COMPASS, QMC_REG_CTRL2, 0x80);     // softreset
    delay(10);
    w &= _write_reg(PDA2_I2C_COMPASS, QMC_REG_PERIOD, 0x01);    // SET/RESET period (по даташиту)
    w &= _write_reg(PDA2_I2C_COMPASS, QMC_REG_CTRL1, 0x19);     // OSR512, ±8G, 100 Гц, continuous
    if (!w) {
        PDA_LOGE("imu", "QMC5883L config failed (I2C write error)");
        return false;
    }
    PDA_LOGI("imu", "QMC5883L ok (±8G, 100Hz)");
    return true;
}

void Imu_Class::begin() {
    _ok = _bmi_begin();

    _mag_fail = 0;
    _mag_present = _mag_begin();
    if (!_mag_present) PDA_LOGI("imu", "QMC5883L not found (optional)");
    _load_mag_offset();

    update();       // первый снимок сразу, чтобы get() не отдавал пустоту
}

void Imu_Class::update() {
    if (_ok) {
        uint8_t buf[12];
        if (_read_bytes(PDA2_I2C_IMU, BMI160_REG_GYRO_DATA, buf, 12)) {
            int16_t gx = (int16_t)(buf[1]  << 8 | buf[0]);
            int16_t gy = (int16_t)(buf[3]  << 8 | buf[2]);
            int16_t gz = (int16_t)(buf[5]  << 8 | buf[4]);
            int16_t ax = (int16_t)(buf[7]  << 8 | buf[6]);
            int16_t ay = (int16_t)(buf[9]  << 8 | buf[8]);
            int16_t az = (int16_t)(buf[11] << 8 | buf[10]);
            _set_accel_gyro(ax * ACCEL_SCALE, ay * ACCEL_SCALE, az * ACCEL_SCALE,
                            gx * GYRO_SCALE,  gy * GYRO_SCALE,  gz * GYRO_SCALE);
        } else {
            _accel_gyro_failed();
        }
    }

    if (_mag_present) {
        uint8_t m[7];
        bool bus_ok = _read_bytes(PDA2_I2C_COMPASS, QMC_REG_DATA, m, 7);
        _mag_bus_result(bus_ok);
        // Нет новых данных или переполнение — оставляем прошлый отсчёт
        if (bus_ok && (m[6] & QMC_STATUS_DRDY) && !(m[6] & QMC_STATUS_OVL)) {
            int16_t x = (int16_t)(m[1] << 8 | m[0]);
            int16_t y = (int16_t)(m[3] << 8 | m[2]);
            int16_t z = (int16_t)(m[5] << 8 | m[4]);
            _set_mag_raw(x, y, z);
        }
    }
}

#endif // !PDA2_SIM