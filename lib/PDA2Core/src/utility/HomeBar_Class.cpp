#include "HomeBar_Class.h"
#include "../pda2_platform.h"
#include "../src/ui/pda2_ui_tokens.h"

// Геометрия из Figma (pda2_ui.md, «Открыто по UI»), радиус 4 — со слов автора.
namespace {
constexpr int32_t  BAR_W      = 114;
constexpr int32_t  BAR_H      = 5;
constexpr int32_t  BAR_X      = 103;
constexpr int32_t  BAR_Y      = 464;
constexpr int32_t  BAR_RADIUS = 4;

// Плавность: постоянная времени сглаживания, мс (меньше = резче).
constexpr int32_t  TAU_FOLLOW_MS = 55;   // палец на экране
constexpr int32_t  TAU_RETURN_MS = 70;   // возврат на место после отпускания
constexpr int32_t  MAX_DT_MS     = 50;   // потолок dt: после подвисания не прыгать
constexpr int32_t  SUB           = 16;   // подпиксельная точность (1/16 px)
}

void HomeBar_Class::begin() {
    _bar = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_bar, BAR_W, BAR_H);
    lv_obj_set_pos(_bar, BAR_X, BAR_Y);
    lv_obj_set_style_bg_color(_bar, lv_color_hex(UI_COLOR_LIGHT), 0);
    lv_obj_set_style_bg_opa(_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_bar, 0, 0);
    lv_obj_set_style_radius(_bar, BAR_RADIUS, 0);
    lv_obj_set_style_pad_all(_bar, 0, 0);
    lv_obj_remove_flag(_bar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(_bar, LV_OBJ_FLAG_SCROLLABLE);
}

void HomeBar_Class::drag(int32_t dy) {
    if (dy < 0) dy = 0;
    _dragging = true;
    _target16 = dy * SUB;
}

void HomeBar_Class::release() {
    _dragging = false;
    _target16 = 0;
}

void HomeBar_Class::update() {
    if (!_bar) return;

    // Время фиксируем каждый вызов, чтобы dt не накапливался в простое
    const uint32_t now = pda2_platform_now_ms();
    int32_t dt = (int32_t)(now - _last_ms);
    _last_ms = now;

    if (_cur16 == _target16) return;
    if (dt > MAX_DT_MS) dt = MAX_DT_MS;

    const int32_t diff = _target16 - _cur16;
    if (diff > -SUB && diff < SUB) {
        _cur16 = _target16;                      // осталось меньше 1 px — встать точно
    } else {
        const int32_t tau  = _dragging ? TAU_FOLLOW_MS : TAU_RETURN_MS;
        int32_t       step = diff * dt / (tau + dt);
        if (step == 0 && dt > 0) step = (diff > 0) ? 1 : -1;
        _cur16 += step;
    }

    const int32_t px = (_cur16 + SUB / 2) / SUB;  // _cur16 >= 0 — округление корректно
    if (px != _applied) {
        _applied = px;
        lv_obj_set_y(_bar, BAR_Y - px);
    }
}