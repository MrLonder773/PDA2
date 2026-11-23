#pragma once
#include <stdint.h>

// Общие цветовые токены UI (формат 0xRRGGBB, для lv_color_hex)
constexpr uint32_t UI_COLOR_BG    = 0x1a2438;  // фон экрана, pager, страниц
constexpr uint32_t UI_COLOR_TEXT  = 0xFFFFFF;  // текст, рамка выбора
constexpr uint32_t UI_COLOR_LIGHT = 0xD9D9D9;  // активная точка, полоса home