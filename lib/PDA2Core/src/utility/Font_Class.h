#pragma once
// ═══════════════════════════════════════════════════════════════
// Font_Class — утилита для получения кириллических шрифтов по размеру
// lib/PDA2Core/src/utility/Font_Class.h
// PDA 2 v2.0.11
// ═══════════════════════════════════════════════════════════════

#include <lvgl.h>

LV_FONT_DECLARE(pda2_cyrillic_12);
LV_FONT_DECLARE(pda2_cyrillic_14);
LV_FONT_DECLARE(pda2_cyrillic_16);
LV_FONT_DECLARE(pda2_cyrillic_20);
LV_FONT_DECLARE(pda2_cyrillic_24);

class Font_Class {
public:
    // Возвращает указатель на шрифт по размеру.
    // Неизвестный размер → pda2_cyrillic_16.
    // Использование: lv_obj_set_style_text_font(lbl, PDA.Fonts.get(24), 0);
    const lv_font_t* get(uint8_t size) const {
        switch (size) {
            case 12: return &pda2_cyrillic_12;
            case 14: return &pda2_cyrillic_14;
            case 16: return &pda2_cyrillic_16;
            case 20: return &pda2_cyrillic_20;
            case 24: return &pda2_cyrillic_24;
            default: return &pda2_cyrillic_16;
        }
    }
};