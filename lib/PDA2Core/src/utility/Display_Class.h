#pragma once

#include <stdint.h>

#ifndef PDA2_SIM

namespace lgfx { inline namespace v1 { class LGFX_Device; } }   // inline — обязательно, иначе конфликт с реальным объявлением в LovyanGFX

#else

// Минимальная PC-совместимая замена LGFX_Device для raw().
// Покрывает только то, что реально используется вызывающим кодом
// (render.cpp в GLTestApp: pushImage) — не полный аналог LGFX_Device.
// pushImage() пишет пиксели напрямую в SDL2-текстуру, мимо LVGL-буфера —
// тот же эффект, что и прямой SPI-пуш на железе (см. Display_Class_sim.cpp).
class PdaRawDisplay_Sim {
public:
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* data);
};

#endif

class Display_Class {
public:
    bool    begin(uint8_t brightness, uint8_t rotation);

    void    setBrightness(uint8_t v);
    uint8_t getBrightness();

    void    setRotation(uint8_t r);
    uint8_t getRotation();

    int     width();
    int     height();

    // ⚠️ Прямой доступ к display-объекту, в обход общей абстракции Display_Class.
    // Осознанное исключение из ПРАВИЛА 4 — только для GLTestApp (см. session doc).
    // Тип раздваивается по платформе: на ESP32 — реальный LGFX_Device,
    // на симуляторе — PdaRawDisplay_Sim с тем же используемым методом
    // (pushImage). Вызывающий код (render.cpp) берёт тип через auto&,
    // поэтому ему всё равно, какая из двух реализаций подставлена.
#ifndef PDA2_SIM
    lgfx::v1::LGFX_Device& raw();
#else
    PdaRawDisplay_Sim& raw();
#endif

private:
    uint8_t _brightness = 200;
    uint8_t _rotation   = 2;
};