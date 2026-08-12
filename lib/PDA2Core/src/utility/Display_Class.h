#pragma once

#include <stdint.h>

namespace lgfx { inline namespace v1 { class LGFX_Device; } }   // inline — обязательно, иначе конфликт с реальным объявлением в LovyanGFX

class Display_Class {
public:
    bool    begin(uint8_t brightness, uint8_t rotation);

    void    setBrightness(uint8_t v);
    uint8_t getBrightness();

    void    setRotation(uint8_t r);
    uint8_t getRotation();

    int     width();
    int     height();

    // ⚠️ Прямой доступ к LGFX-объекту, в обход общей абстракции Display_Class.
    // Осознанное исключение из ПРАВИЛА 4 — только для GLTestApp (см. session doc).
    lgfx::v1::LGFX_Device& raw();

private:
    uint8_t _brightness = 200;
    uint8_t _rotation   = 2;
};