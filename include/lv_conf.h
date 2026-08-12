#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

// ═══════════════════════════════════════════════
// LVGL 9.2.1 — PDA Core config
// ESP32-S3 N16R8
// ═══════════════════════════════════════════════

#define LV_COLOR_DEPTH 16

// ── Память (PSRAM через malloc) ──────────────────
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING    LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_CLIB

// ── OS ──────────────────────────────────────────
#define LV_USE_OS   LV_OS_FREERTOS

// ── DPI ─────────────────────────────────────────
#define LV_DPI_DEF  130

// ── Встроенные шрифты (латиница + цифры) ────────
#define LV_FONT_MONTSERRAT_10  1
#define LV_FONT_MONTSERRAT_12  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  1
#define LV_FONT_MONTSERRAT_18  1
#define LV_FONT_MONTSERRAT_20  1
#define LV_FONT_MONTSERRAT_24  1
#define LV_FONT_MONTSERRAT_48  1

#define LV_FONT_DEFAULT  &lv_font_montserrat_14

// ── Кастомные шрифты (кириллица + ASCII + стрелки)
//    Файлы: lib/PDA2Core/src/fonts/pda2_cyrillic_*.c
#define LV_FONT_CUSTOM_DECLARE       \
    extern const lv_font_t pda2_cyrillic_12; \
    extern const lv_font_t pda2_cyrillic_14; \
    extern const lv_font_t pda2_cyrillic_16; \
    extern const lv_font_t pda2_cyrillic_20; \
    extern const lv_font_t pda2_cyrillic_24;

// ── Анимации ────────────────────────────────────
#define LV_USE_ANIM  1   // в LVGL 9.x — LV_USE_ANIM, не LV_USE_ANIMATION

// ── Виджеты (LVGL 9.x — переименованы) ──────────
#define LV_USE_BUTTON    1   // было LV_USE_BTN
#define LV_USE_LABEL     1
#define LV_USE_IMAGE     1   // было LV_USE_IMG
#define LV_USE_LINE      1
#define LV_USE_ARC       1
#define LV_USE_BAR       1
#define LV_USE_SLIDER    1
#define LV_USE_SWITCH    1
#define LV_USE_TEXTAREA  1
#define LV_USE_TABLE     1
#define LV_USE_LIST      1
#define LV_USE_ROLLER    1
#define LV_USE_DROPDOWN  1
#define LV_USE_CHART     0
#define LV_USE_CALENDAR  0
#define LV_USE_KEYBOARD  1
#define LV_USE_MSGBOX    1
#define LV_USE_SPINNER   1
#define LV_USE_TABVIEW   1
#define LV_USE_TILEVIEW  1

// ── Лог и assert ────────────────────────────────
#define LV_USE_LOG      1
#define LV_LOG_LEVEL    LV_LOG_LEVEL_WARN
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0

#endif /* LV_CONF_H */