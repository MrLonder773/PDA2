#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — Display_Class.h
//  ILI9488 + LovyanGFX + LVGL 9.x
//  RENDER_MODE_FULL, один буфер в PSRAM (buf2=nullptr). Причина: BL-1.
// ════════════════════════════════════════════════════════

#include <stdint.h>

class Display_Class {
public:
    bool    begin(uint8_t brightness, uint8_t rotation);  // [A-06]

    // Яркость (0–255). Сохранение в prefs — на стороне PDA2Class.
    void    setBrightness(uint8_t v);
    uint8_t getBrightness();

    // Ротация (0–3). Обновляет и LovyanGFX, и LVGL.
    void    setRotation(uint8_t r);
    uint8_t getRotation();

    int     width();
    int     height();

private:
    uint8_t _brightness = 200;
    uint8_t _rotation   = 2;
};