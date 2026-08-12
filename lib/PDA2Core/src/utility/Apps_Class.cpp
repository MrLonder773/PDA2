// ════════════════════════════════════════════════════════
//  PDA 2 — Apps_Class.cpp
// ════════════════════════════════════════════════════════

#include <PDA2.h>
#include "Apps_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"
#include <Arduino.h>

extern const lv_font_t pda2_cyrillic_16;
static Apps_Class* _apps_instance = nullptr;

// ── Цвета иконок (по кругу) ──────────────────────────────
static const uint32_t _icon_colors[] = {
    0x2563eb,  // синий
    0x16a34a,  // зелёный
    0x9333ea,  // фиолетовый
    0x6b7280,  // серый
    0x0891b2,  // голубой
    0xf97316,  // оранжевый
    0x0ea5e9,  // небесный
    0xef4444,  // красный
    0x8b5cf6,  // лавандовый
    0x14b8a6,  // бирюзовый
    0xdc2626,  // тёмно-красный
    0x22c55e,  // ярко-зелёный
    0xe879f9,  // розовый
    0xfbbf24,  // жёлтый
    0x34d399,  // мятный
    0xf472b6,  // светло-розовый
};
static const uint8_t _n_colors = sizeof(_icon_colors) / sizeof(_icon_colors[0]);

// ── add / start ──────────────────────────────────────────
void Apps_Class::add(PDA2App* app) {
    if (_count >= PDA2_MAX_APPS) {
        PDA_LOGE("apps", "Max apps reached (%d)", PDA2_MAX_APPS);
        return;
    }
    _apps[_count++] = app;
    PDA_LOGI("apps", "Registered: %s [%d]", app->name, _count - 1);
}

void Apps_Class::start() {
    _apps_instance = this;

    for (uint8_t i = 0; i < _count; i++) {
        if (_apps[i]) {
            _apps[i]->onInit();
            PDA_LOGI("apps", "onInit: %s", _apps[i]->name);
        }
    }

    _build_launcher();
    lv_screen_load(_screen_launcher);

    PDA_LOGI("apps", "Launcher ready. %d app(s) registered.", _count);
}

// ── open / close ─────────────────────────────────────────
void Apps_Class::open(uint8_t id) {
    if (id >= _count || !_apps[id] || !_apps[id]->screen) {
        PDA_LOGE("apps", "open(%d): invalid id", id);
        return;
    }
    _current_id = id;
    _is_open    = true;

    lv_screen_load_anim(
        _apps[id]->screen,
        LV_SCR_LOAD_ANIM_MOVE_LEFT,
        PDA2_ANIM_MS, 0, false
    );

    _apps[id]->onOpen();
    PDA_LOGI("apps", "Opened: %s", _apps[id]->name);
}

void Apps_Class::close() {
    if (!_is_open || _current_id < 0) return;

    _apps[_current_id]->onClose();
    PDA_LOGI("apps", "Closed: %s", _apps[_current_id]->name);

    _current_id = -1;
    _is_open    = false;

    lv_screen_load_anim(
        _screen_launcher,
        LV_SCR_LOAD_ANIM_MOVE_RIGHT,
        PDA2_ANIM_MS, 0, false
    );
}

// ── tick ─────────────────────────────────────────────────
void Apps_Class::tick(uint32_t delta_ms) {
    _clock_last_ms += delta_ms;
    if (_clock_last_ms >= 1000) {
        _clock_last_ms = 0;
        if (PDA.Rtc.ok()) {
            pda2_time_t t = PDA.Rtc.get();
            char buf[6];
            snprintf(buf, sizeof(buf), "%02d:%02d", t.hour, t.minute);
            lv_label_set_text(_lbl_time, buf);
        }
    }

    if (_is_open && _current_id >= 0 && _apps[_current_id]) {
        _apps[_current_id]->onTick(delta_ms);
    }

    for (uint8_t i = 0; i < _count; i++) {
        if ((int8_t)i == _current_id) continue;
        if (_apps[i]) _apps[i]->onBackground();
    }

    _tick_toast((uint32_t)millis());
    lv_timer_handler();
}

// ── getters ──────────────────────────────────────────────
PDA2App* Apps_Class::current() {
    if (_current_id >= 0) return _apps[_current_id];
    return nullptr;
}
bool    Apps_Class::isOpen()  { return _is_open; }
uint8_t Apps_Class::count()   { return _count; }

// ── showToast ─────────────────────────────────────────────
void Apps_Class::showToast(const char* text, uint32_t ms) {
    if (!_toast_cont || !_toast_label) return;
    lv_label_set_text(_toast_label, text);
    lv_obj_clear_flag(_toast_cont, LV_OBJ_FLAG_HIDDEN);
    // lv_obj_move_foreground(_toast_cont);  ← удалить
    _toast_until = millis() + ms;
    PDA_LOGI("apps", "Toast: %s (%ums)", text, ms);
}

void Apps_Class::_tick_toast(uint32_t now) {
    if (!_toast_cont) return;
    if (lv_obj_has_flag(_toast_cont, LV_OBJ_FLAG_HIDDEN)) return;
    if (now >= _toast_until) {
        lv_obj_add_flag(_toast_cont, LV_OBJ_FLAG_HIDDEN);
    }
}

// ── Launcher button callback ──────────────────────────────
void Apps_Class::_btn_cb(lv_event_t* e) {
    if (!_apps_instance) return;
    uint8_t id = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    _apps_instance->open(id);
}

// ── Build launcher ────────────────────────────────────────
void Apps_Class::_build_launcher() {
    // Экран
    _screen_launcher = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(_screen_launcher, lv_color_hex(0x1e2235), 0);
    lv_obj_set_style_bg_opa(_screen_launcher, LV_OPA_COVER, 0);
    lv_obj_clear_flag(_screen_launcher, LV_OBJ_FLAG_SCROLLABLE);

    // Статус-бар
    lv_obj_t* bar = lv_obj_create(_screen_launcher);
    lv_obj_set_size(bar, LV_HOR_RES, 28);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_hor(bar, 14, 0);
    lv_obj_set_style_pad_ver(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_time = lv_label_create(bar);
    lv_label_set_text(_lbl_time, "--:--");
    lv_obj_set_style_text_color(_lbl_time, lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(_lbl_time, &lv_font_montserrat_16, 0);
    lv_obj_set_style_bg_opa(_lbl_time, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(_lbl_time, lv_color_hex(0x1e2235), 0);
    lv_obj_align(_lbl_time, LV_ALIGN_CENTER, 0, 0);

    // Сетка приложений — flex row wrap
    //   Иконка: PDA2_ICON_SIZE × PDA2_ICON_SIZE, 4 в ряд (PDA2_GRID_COLS)
    //   gap 16, pad 14

    static const lv_coord_t GAP = 16;

    lv_obj_t* grid = lv_obj_create(_screen_launcher);
    lv_obj_set_size(grid, LV_HOR_RES, LV_VER_RES - 28);
    lv_obj_align(grid, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_left(grid, 14, 0);
    lv_obj_set_style_pad_right(grid, 14, 0);
    lv_obj_set_style_pad_top(grid, 14, 0);
    lv_obj_set_style_pad_bottom(grid, 14, 0);
    lv_obj_set_style_pad_row(grid, GAP, 0);
    lv_obj_set_style_pad_column(grid, GAP, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    for (uint8_t i = 0; i < _count; i++) {
        // Ячейка (иконка + подпись)
        lv_obj_t* cell = lv_obj_create(grid);
        lv_obj_set_size(cell, PDA2_ICON_SIZE, PDA2_ICON_SIZE + 24);
        lv_obj_set_style_bg_opa(cell, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(cell, 0, 0);
        lv_obj_set_style_pad_all(cell, 0, 0);
        lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_layout(cell, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // Иконка — кнопка
        lv_obj_t* btn = lv_btn_create(cell);
        lv_obj_set_size(btn, PDA2_ICON_SIZE, PDA2_ICON_SIZE);
        lv_obj_set_style_bg_color(btn, lv_color_hex(_icon_colors[i % _n_colors]), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn, 14, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_add_event_cb(btn, _btn_cb, LV_EVENT_CLICKED,
                            (void*)(uintptr_t)i);
        _icon_btns[i] = btn;
        // Начальное состояние рамки
        lv_obj_set_style_border_color(btn, lv_color_white(), 0);
        // Первая буква имени как заглушка иконки
        char letter[2] = { _apps[i]->name[0], '\0' };
        lv_obj_t* lbl_icon = lv_label_create(btn);
        lv_label_set_text(lbl_icon, letter);
        lv_obj_set_style_text_color(lbl_icon, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_text_font(lbl_icon, &lv_font_montserrat_20, 0);
        lv_obj_set_style_bg_opa(lbl_icon, LV_OPA_TRANSP, 0);
        lv_obj_align(lbl_icon, LV_ALIGN_CENTER, 0, 0);

        // Подпись под иконкой
        lv_obj_t* lbl_name = lv_label_create(cell);
        lv_label_set_text(lbl_name, _apps[i]->name);
        lv_obj_set_style_text_color(lbl_name, lv_color_hex(0xdddddd), 0);
        lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_12, 0);
        lv_obj_set_style_bg_opa(lbl_name, LV_OPA_TRANSP, 0);
        lv_obj_set_style_text_align(lbl_name, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_long_mode(lbl_name, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl_name, PDA2_ICON_SIZE);
        lv_obj_set_style_pad_top(lbl_name, 4, 0);
    }
    _updateLauncherSelection();
   // Toast
    lv_obj_t* layer = lv_layer_top();

    _toast_cont = lv_obj_create(layer);
    lv_obj_set_size(_toast_cont, lv_pct(70), LV_SIZE_CONTENT);
    lv_obj_align(_toast_cont, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_bg_color(_toast_cont, lv_color_hex(0x1e3a5f), 0);
    lv_obj_set_style_bg_opa(_toast_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(_toast_cont, 12, 0);
    lv_obj_set_style_pad_all(_toast_cont, 10, 0);
    lv_obj_set_style_border_width(_toast_cont, 0, 0);
    lv_obj_add_flag(_toast_cont, LV_OBJ_FLAG_HIDDEN);

    _toast_label = lv_label_create(_toast_cont);
    lv_obj_set_width(_toast_label, lv_pct(100));
    lv_label_set_long_mode(_toast_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(_toast_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(_toast_label, &pda2_cyrillic_16, 0);
    lv_obj_set_style_text_align(_toast_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_opa(_toast_label, LV_OPA_TRANSP, 0);
    lv_obj_align(_toast_label, LV_ALIGN_CENTER, 0, 0);
}

// ── next / prev / openSelected ────────────────────────────
void Apps_Class::next() {
    if (_count == 0) return;
    _selected_id = (_selected_id + 1) % _count;
    _updateLauncherSelection();
}

void Apps_Class::prev() {
    if (_count == 0) return;
    _selected_id = (_selected_id - 1 + _count) % _count;
    _updateLauncherSelection();
}

void Apps_Class::openSelected() {
    if (_selected_id >= 0 && _selected_id < _count)
        open((uint8_t)_selected_id);
}

// ── Подсветка выбранной иконки ────────────────────────────
void Apps_Class::_updateLauncherSelection() {
    for (uint8_t i = 0; i < _count; i++) {
        if (!_icon_btns[i]) continue;
        lv_obj_set_style_border_width(_icon_btns[i],
            (i == (uint8_t)_selected_id) ? 2 : 0, 0);
    }
}

// ── Клавиатурный колбэк (USB + BLE) ──────────────────────
void Apps_Class::_key_cb(uint8_t keycode, uint8_t modifier, bool pressed) {
    if (!pressed || !_apps_instance) return;

    if (_apps_instance->isOpen()) {
        if (keycode == 41) {           // Esc → закрыть приложение
            _apps_instance->close();
        } else {
            PDA2App* app = _apps_instance->current();
            if (app) app->onKey(keycode, modifier);
        }
    } else {
        // Launcher: навигация по иконкам
        switch (keycode) {
            case 80: case 82: _apps_instance->prev();         break; // ← ↑
            case 79: case 81: _apps_instance->next();         break; // → ↓
            case 40:          _apps_instance->openSelected(); break; // Enter
        }
    }
}