// ════════════════════════════════════════════════════════
//  PDA 2 — pda2_platform_esp32.cpp
//  Платформенный слой для реального железа (ESP32-S3).
//  Компилируется, только если PDA2_SIM НЕ определён —
//  в сборке симулятора (-DPDA2_SIM) тело пустое.
// ════════════════════════════════════════════════════════

#include "pda2_platform.h"

#ifndef PDA2_SIM

#include <Arduino.h>
#include <Wire.h>
#include <esp_heap_caps.h>
#include "pda2_config.h"
#include "pda2_log.h"

void pda2_platform_begin() {
    Serial.begin(115200);
    delay(200);
}

void pda2_platform_i2c_begin(uint8_t sda, uint8_t scl) {
    Wire.begin(sda, scl);
    Wire.setClock(100000);
}

uint32_t pda2_platform_now_ms() {
    return millis();
}

void pda2_platform_sleep_ms(uint32_t ms) {
    delay(ms);
}

uint32_t pda2_platform_free_heap() {
    return (uint32_t)heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
}

uint32_t pda2_platform_free_psram() {
    return (uint32_t)heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
}

void* pda2_platform_psram_malloc(size_t size) {
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
}

// ── Touch: FT6236/FT6336U, raw I2C ───────────────────────

bool pda2_platform_touch_begin() {
    pinMode(PDA2_PIN_TOUCH_RST, OUTPUT);
    digitalWrite(PDA2_PIN_TOUCH_RST, LOW);
    delay(10);
    digitalWrite(PDA2_PIN_TOUCH_RST, HIGH);
    delay(100);

    Wire.beginTransmission(PDA2_I2C_TOUCH);
    uint8_t err = Wire.endTransmission();
    if (err != 0) return false;

    Wire.beginTransmission(PDA2_I2C_TOUCH);
    Wire.write(0xA3);   // Chip ID: 0x64 = FT6336U, 0x36 = FT6236
    if (Wire.endTransmission(false) == 0 &&
        Wire.requestFrom((uint8_t)PDA2_I2C_TOUCH, (uint8_t)1) == 1) {
        PDA_LOGI("touch", "chip id 0x%02X", Wire.read());
    }
    return true;
}

bool pda2_platform_touch_read(int32_t& x, int32_t& y) {
    Wire.beginTransmission(PDA2_I2C_TOUCH);
    Wire.write(0x02);   // TD_STATUS
    if (Wire.endTransmission(false) != 0) return false;
    Wire.requestFrom((uint8_t)PDA2_I2C_TOUCH, (uint8_t)5);
    if (Wire.available() < 5) return false;

    uint8_t td  = Wire.read();    // 0x02 — число точек
    uint8_t xh  = Wire.read();    // 0x03
    uint8_t xl  = Wire.read();    // 0x04
    uint8_t yh  = Wire.read();    // 0x05
    uint8_t yl  = Wire.read();    // 0x06

    if ((td & 0x0F) == 0) return false;

    x = ((int32_t)(xh & 0x0F) << 8) | xl;
    y = ((int32_t)(yh & 0x0F) << 8) | yl;
    return true;
}

uint8_t pda2_platform_touch_read2(Pda2TouchPoint pts[2]) {
    Wire.beginTransmission(PDA2_I2C_TOUCH);
    Wire.write(0x02);
    if (Wire.endTransmission(false) != 0) return 0;
    if (Wire.requestFrom((uint8_t)PDA2_I2C_TOUCH, (uint8_t)11) != 11) return 0;
    uint8_t b[11];
    for (uint8_t i = 0; i < 11; i++) b[i] = Wire.read();

    uint8_t n = b[0] & 0x0F;          // число точек
    if (n > 2) n = 2;
    uint8_t cnt = 0;
    for (uint8_t s = 0; s < n; s++) {
        const uint8_t* p = &b[s == 0 ? 1 : 7];   // слот 1: 0x03..0x06, слот 2: 0x09..0x0C
        if (p[0] == 0xFF && p[1] == 0xFF && p[2] == 0xFF && p[3] == 0xFF) continue;
        pts[cnt].x  = ((int32_t)(p[0] & 0x0F) << 8) | p[1];
        pts[cnt].y  = ((int32_t)(p[2] & 0x0F) << 8) | p[3];
        pts[cnt].id = p[2] >> 4;
        pts[cnt].ev = p[0] >> 6;
        cnt++;
    }
    return cnt;
}

bool pda2_platform_should_quit() {
    return false; // на железе некуда "закрывать окно"
}

void pda2_platform_request_quit() {
    // Пусто — на ESP32 вызывать неоткуда.
}

#endif // !PDA2_SIM