// ════════════════════════════════════════════════════════
//  PDA 2 — Imu_Class.cpp
//  BMI160: chip_id=0xD1, accel 0x12, gyro 0x0C.
// ════════════════════════════════════════════════════════

#include "Imu_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"
#include <Wire.h>
#include <Arduino.h>

#define BMI160_REG_CHIP_ID    0x00
#define BMI160_REG_CMD        0x7E
#define BMI160_REG_ACC_RANGE  0x41
#define BMI160_REG_GYR_RANGE  0x43
#define BMI160_REG_GYRO_DATA  0x0C
#define BMI160_REG_ACCEL_DATA 0x12
#define BMI160_CHIP_ID        0xD1

// Диапазон: акселерометр ±2g, гироскоп ±250°/s
#define ACCEL_SCALE  (2.0f / 32768.0f)
#define GYRO_SCALE   (250.0f / 32768.0f)

static void _write_reg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(PDA2_I2C_IMU);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

static uint8_t _read_reg(uint8_t reg) {
    Wire.beginTransmission(PDA2_I2C_IMU);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)PDA2_I2C_IMU, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0;
}

static void _read_bytes(uint8_t reg, uint8_t* buf, uint8_t len) {
    Wire.beginTransmission(PDA2_I2C_IMU);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)PDA2_I2C_IMU, len);
    for (uint8_t i = 0; i < len && Wire.available(); i++) {
        buf[i] = Wire.read();
    }
}

void Imu_Class::begin() {
    uint8_t id = _read_reg(BMI160_REG_CHIP_ID);
    if (id != BMI160_CHIP_ID) {
        PDA_LOGE("imu", "BMI160 not found (id=0x%02X)", id);
        _ok = false;
        return;
    }

    // Softreset
    _write_reg(BMI160_REG_CMD, 0xB6);
    delay(100);

    // Включить акселерометр — normal mode
    _write_reg(BMI160_REG_CMD, 0x11);
    delay(100);

    // Включить гироскоп — normal mode
    _write_reg(BMI160_REG_CMD, 0x15);
    delay(100);

    // Явно выставить диапазоны (не полагаться на дефолты после reset)
    _write_reg(BMI160_REG_ACC_RANGE, 0x03);  // ±2g
    _write_reg(BMI160_REG_GYR_RANGE, 0x03);  // ±250°/s
    delay(10);

    _ok = true;
    PDA_LOGI("imu", "BMI160 ok (id=0xD1, accel=±2g, gyro=±250dps)");
}

bool Imu_Class::ok() { return _ok; }

pda2_imu_t Imu_Class::get() {
    pda2_imu_t d = {};
    if (!_ok) return d;

    uint8_t buf[12];
    _read_bytes(BMI160_REG_GYRO_DATA, buf, 12);

    int16_t gx = (int16_t)(buf[1]  << 8 | buf[0]);
    int16_t gy = (int16_t)(buf[3]  << 8 | buf[2]);
    int16_t gz = (int16_t)(buf[5]  << 8 | buf[4]);
    int16_t ax = (int16_t)(buf[7]  << 8 | buf[6]);
    int16_t ay = (int16_t)(buf[9]  << 8 | buf[8]);
    int16_t az = (int16_t)(buf[11] << 8 | buf[10]);

    d.gx = gx * GYRO_SCALE;
    d.gy = gy * GYRO_SCALE;
    d.gz = gz * GYRO_SCALE;
    d.ax = ax * ACCEL_SCALE;
    d.ay = ay * ACCEL_SCALE;
    d.az = az * ACCEL_SCALE;

    return d;
}