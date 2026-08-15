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

// ── Touch: FT6236, raw I2C ───────────────────────────────
#include "pda2_config.h"

bool pda2_platform_touch_begin() {
    pinMode(PDA2_PIN_TOUCH_RST, OUTPUT);
    digitalWrite(PDA2_PIN_TOUCH_RST, LOW);
    delay(10);
    digitalWrite(PDA2_PIN_TOUCH_RST, HIGH);
    delay(100);

    Wire.beginTransmission(PDA2_I2C_TOUCH);
    uint8_t err = Wire.endTransmission();
    return err == 0;
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

bool pda2_platform_should_quit() {
    return false; // на железе некуда "закрывать окно"
}

void pda2_platform_request_quit() {
    // Пусто — на ESP32 вызывать неоткуда.
}

#endif // !PDA2_SIM