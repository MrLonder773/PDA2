#pragma once

#include <stdint.h>

#ifndef PDA2_SIM

namespace lgfx { inline namespace v1 {
    class LGFX_Device;   // inline — обязательно, иначе конфликт с реальным объявлением в LovyanGFX
    struct rgb565_t;     // forward-declare, полное определение — в LovyanGFX.hpp (только .cpp, куда он реально нужен)
} }

// Тип указателя на пиксель для writePixelsDMA() — платформенный.
// TinyGLTestApp приводит свой буфер к этому типу без #ifdef.
using pda2_pixel_ptr_t = lgfx::rgb565_t*;

#else

// Минимальная PC-совместимая замена LGFX_Device для raw().
// Повторяет низкоуровневый путь LVGL flush_cb: startWrite/setAddrWindow/
// writePixelsDMA/endWrite. Высокоуровневый pushImage() не используется:
// он сам конвертирует пиксели и на железе даёт неверные цвета.
class PdaRawDisplay_Sim {
public:
    void startWrite();
    void setAddrWindow(int32_t x, int32_t y, int32_t w, int32_t h);
    void writePixelsDMA(const uint16_t* data, uint32_t len);
    void endWrite();

private:
    int32_t _win_x = 0, _win_y = 0, _win_w = 0, _win_h = 0;
};

using pda2_pixel_ptr_t = uint16_t*;

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

    // ⚠️ Прямой доступ к display-объекту в обход Display_Class.
    // Исключение из ПРАВИЛА 4 — только для TinyGLTestApp.
    // Тип зависит от платформы: на ESP32 реальный LGFX_Device, на симуляторе
    // PdaRawDisplay_Sim с теми же методами (startWrite/setAddrWindow/
    // writePixelsDMA/endWrite), поэтому вызывающий код одинаков на обеих.
#ifndef PDA2_SIM
    lgfx::v1::LGFX_Device& raw();
#else
    PdaRawDisplay_Sim& raw();
#endif

private:
    uint8_t _brightness = 200;
    uint8_t _rotation   = 2;
};