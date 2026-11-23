// ════════════════════════════════════════════════════════
//  PDA 2 — Apps_Launcher.cpp
//  Постройка launcher: горизонтальный pager со snap-прокруткой,
//  страницы шириной в экран, сетка иконок, точки, подсветка выбора.
//  Сетка — абсолютные позиции из Figma (4×4 на страницу).
//  Палец двигает pager покадрово, в середине движения видны
//  половины двух страниц; доводку и привязку делает LVGL.
//  Время показывает StatusBar, не launcher.
// ════════════════════════════════════════════════════════

#include <PDA2.h>
#include "Apps_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"

// ── Геометрия (Figma) ────────────────────────────────────
static constexpr int32_t SCREEN_W     = 320;
static constexpr int32_t SCREEN_H     = 480;
static constexpr int32_t GRID_COLS    = 4;
static constexpr int32_t GRID_ROWS    = 4;
static constexpr int32_t GRID_X0      = 16;
static constexpr int32_t GRID_STEP_X  = 76;                       // 16, 92, 168, 244
static constexpr int32_t ROW_Y[GRID_ROWS] = {44, 144, 244, 344};  // шаг по y = 100
static constexpr int32_t LABEL_DY     = 64;                       // верх подписи = y иконки + 64
static constexpr int32_t LABEL_W      = GRID_STEP_X;              // по центру ячейки, перенос по словам
static constexpr int32_t PER_PAGE     = PDA2_APPS_PER_PAGE;

static_assert(PER_PAGE <= GRID_COLS * GRID_ROWS,
              "PDA2_APPS_PER_PAGE больше, чем ячеек в сетке");

// ── Точки страниц (Figma): 6×6, шаг 9, y432, по центру ───
static constexpr int32_t DOT_SIZE     = 6;
static constexpr int32_t DOT_STEP     = 9;
static constexpr int32_t DOT_Y        = 432;
static constexpr uint32_t DOT_IDLE    = 0x8d8d8d;

// ── Цвета иконок (по кругу) ──────────────────────────────
static const uint32_t _icon_colors[] = {
    0x2d64f0, 0x26a43d, 0x7732d2, 0x7e7e7e,
    0x0fbbd9, 0xf97316, 0x0ea5e9, 0xef4444,
    0x8b5cf6, 0x14b8a6, 0xdc2626, 0x22c55e,
    0xe879f9, 0xfbbf24, 0x34d399, 0xf472b6,
};
static const uint8_t _n_colors = sizeof(_icon_colors) / sizeof(_icon_colors[0]);

// ── Launcher button callback ──────────────────────────────
void Apps_Class::_btn_cb(lv_event_t* e) {
    if (!_instance) return;
    uint8_t id = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    _instance->open(id);
}

// ── Прокрутка pager: точки следуют за пальцем ─────────────
void Apps_Class::_scroll_cb(lv_event_t* e) {
    (void)e;
    if (_instance) _instance->_update_dots();
}

// Прокрутка закончилась: если выбранная иконка осталась на другой
// странице — выбор переходит на первую иконку текущей.
void Apps_Class::_scroll_end_cb(lv_event_t* e) {
    (void)e;
    if (!_instance) return;
    _instance->_update_dots();
    if (_instance->_selected_id / PER_PAGE != _instance->_page) {
        _instance->_selected_id = _instance->_page * PER_PAGE;
        _instance->_updateLauncherSelection();
    }
}

// Страница по смещению прокрутки; перекрашивает точки при смене
void Apps_Class::_update_dots() {
    if (_pages < 2 || !_pager) return;
    int32_t p = (lv_obj_get_scroll_x(_pager) + SCREEN_W / 2) / SCREEN_W;
    if (p < 0) p = 0;
    if (p >= _pages) p = _pages - 1;
    if (p == _page) return;
    _page = (int8_t)p;
    for (int8_t i = 0; i < _pages; i++) {
        if (!_dots[i]) continue;
        lv_obj_set_style_bg_color(_dots[i],
            lv_color_hex(i == _page ? UI_COLOR_LIGHT : DOT_IDLE), 0);
    }
}

// Выбор иконки (клавиши): если она на другой странице — pager
// прокручивается к ней с анимацией.
void Apps_Class::_select(int8_t id) {
    _selected_id = id;
    const int8_t target = id / PER_PAGE;
    if (_pager && target != _page)
        lv_obj_scroll_to_x(_pager, target * SCREEN_W, LV_ANIM_ON);
    _updateLauncherSelection();
}

// ── Build launcher ────────────────────────────────────────
void Apps_Class::_build_launcher() {
    _pages = (int8_t)((_count + PER_PAGE - 1) / PER_PAGE);
    if (_pages < 1) _pages = 1;
    _page = 0;

    for (uint8_t i = 0; i < PDA2_MAX_APPS; i++) {
        _icon_btns[i] = nullptr;
        _dots[i]      = nullptr;
    }

    _screen_launcher = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(_screen_launcher, lv_color_hex(UI_COLOR_BG), 0);
    lv_obj_set_style_bg_opa(_screen_launcher, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(_screen_launcher, 0, 0);
    lv_obj_clear_flag(_screen_launcher, LV_OBJ_FLAG_SCROLLABLE);

    // ── Pager: окно экрана, страницы лежат внутри рядом ──
    _pager = lv_obj_create(_screen_launcher);
    lv_obj_set_size(_pager, SCREEN_W, SCREEN_H);
    lv_obj_set_pos(_pager, 0, 0);
    lv_obj_set_style_bg_color(_pager, lv_color_hex(UI_COLOR_BG), 0);
    lv_obj_set_style_bg_opa(_pager, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_pager, 0, 0);
    lv_obj_set_style_pad_all(_pager, 0, 0);
    lv_obj_set_scroll_dir(_pager, LV_DIR_HOR);
    lv_obj_set_scroll_snap_x(_pager, LV_SCROLL_SNAP_START);
    lv_obj_set_scrollbar_mode(_pager, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(_pager, LV_OBJ_FLAG_SCROLL_ONE);   // один жест — одна страница
    lv_obj_add_event_cb(_pager, _scroll_cb,     LV_EVENT_SCROLL,     nullptr);
    lv_obj_add_event_cb(_pager, _scroll_end_cb, LV_EVENT_SCROLL_END, nullptr);

    for (int8_t p = 0; p < _pages; p++) {
        lv_obj_t* page = lv_obj_create(_pager);
        lv_obj_set_size(page, SCREEN_W, SCREEN_H);
        lv_obj_set_pos(page, p * SCREEN_W, 0);
        lv_obj_set_style_bg_color(page, lv_color_hex(UI_COLOR_BG), 0);
        lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(page, 0, 0);
        lv_obj_set_style_pad_all(page, 0, 0);
        lv_obj_add_flag(page, LV_OBJ_FLAG_SNAPPABLE);
        lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(page, LV_OBJ_FLAG_CLICKABLE);

        const uint8_t first = (uint8_t)(p * PER_PAGE);
        uint8_t last = first + PER_PAGE;
        if (last > _count) last = _count;

        for (uint8_t i = first; i < last; i++) {
            const uint8_t j = i - first;   // позиция в сетке страницы
            const int32_t x = GRID_X0 + (j % GRID_COLS) * GRID_STEP_X;
            const int32_t y = ROW_Y[j / GRID_COLS];

            lv_obj_t* btn = lv_btn_create(page);
            lv_obj_set_size(btn, PDA2_ICON_SIZE, PDA2_ICON_SIZE);
            lv_obj_set_pos(btn, x, y);
            lv_obj_set_style_bg_color(btn, lv_color_hex(_icon_colors[i % _n_colors]), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
            lv_obj_set_style_radius(btn, 14, 0);
            lv_obj_set_style_shadow_width(btn, 0, 0);
            lv_obj_set_style_border_width(btn, 0, 0);
            lv_obj_set_style_pad_all(btn, 0, 0);
            lv_obj_add_event_cb(btn, _btn_cb, LV_EVENT_CLICKED,
                                (void*)(uintptr_t)i);
            _icon_btns[i] = btn;
            lv_obj_set_style_border_color(btn, lv_color_hex(UI_COLOR_TEXT), 0);

            char letter[2] = { _apps[i]->name[0], '\0' };
            lv_obj_t* lbl_icon = lv_label_create(btn);
            lv_label_set_text(lbl_icon, letter);
            lv_obj_set_style_text_color(lbl_icon, lv_color_hex(UI_COLOR_TEXT), 0);
            lv_obj_set_style_text_font(lbl_icon, PDA.Fonts.get(20), 0);
            lv_obj_set_style_bg_opa(lbl_icon, LV_OPA_TRANSP, 0);
            lv_obj_align(lbl_icon, LV_ALIGN_CENTER, 0, 0);

            // Подпись: родитель-страница с opaque-фоном перерисовывается вместе с ней
            lv_obj_t* lbl_name = lv_label_create(page);
            lv_label_set_text(lbl_name, _apps[i]->name);
            lv_obj_set_style_text_color(lbl_name, lv_color_hex(UI_COLOR_TEXT), 0);
            lv_obj_set_style_text_font(lbl_name, PDA.Fonts.get(12), 0);
            lv_obj_set_style_bg_opa(lbl_name, LV_OPA_TRANSP, 0);
            lv_obj_set_style_text_align(lbl_name, LV_TEXT_ALIGN_CENTER, 0);
            lv_label_set_long_mode(lbl_name, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(lbl_name, LABEL_W);
            lv_obj_set_pos(lbl_name, x + (PDA2_ICON_SIZE - LABEL_W) / 2, y + LABEL_DY);
        }
    }

    // ── Точки: на экране поверх pager, при одной странице не рисуются.
    // Статичные позиции, со своей сплошной заливкой (правило 9).
    if (_pages > 1) {
        const int32_t x0 = (SCREEN_W - (DOT_STEP * (_pages - 1) + DOT_SIZE)) / 2;
        for (int8_t p = 0; p < _pages; p++) {
            lv_obj_t* dot = lv_obj_create(_screen_launcher);
            lv_obj_set_size(dot, DOT_SIZE, DOT_SIZE);
            lv_obj_set_pos(dot, x0 + p * DOT_STEP, DOT_Y);
            lv_obj_set_style_bg_color(dot,
                lv_color_hex(p == _page ? UI_COLOR_LIGHT : DOT_IDLE), 0);
            lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
            lv_obj_set_style_radius(dot, DOT_SIZE / 2, 0);
            lv_obj_set_style_border_width(dot, 0, 0);
            lv_obj_set_style_pad_all(dot, 0, 0);
            lv_obj_remove_flag(dot, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
            _dots[p] = dot;
        }
    }

    _updateLauncherSelection();
}

// ── Подсветка выбранной иконки ────────────────────────────
void Apps_Class::_updateLauncherSelection() {
    for (uint8_t i = 0; i < _count; i++) {
        if (!_icon_btns[i]) continue;
        lv_obj_set_style_border_width(_icon_btns[i],
            (i == (uint8_t)_selected_id) ? 2 : 0, 0);
    }
}