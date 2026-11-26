// ════════════════════════════════════════════════════════
//  PDA 2 — Keyboard_Class.cpp (ядро)
//  lib/PDA2Core/src/utility/Keyboard_Class.cpp
//
//  attach/bind, show/hide, очередь событий, страницы, разбор клавиш,
//  колбэки LVGL. Остальное — блок-файлы того же класса:
//    Keyboard_Layout.cpp — карты клавиатуры и внешний вид (_apply_layout)
//    Keyboard_Hints.cpp  — подсказки, долгое нажатие, попап
// ════════════════════════════════════════════════════════

#include "Keyboard_Class.h"
#include "../PDA2.h"
#include <string.h>
#include "../pda2_platform.h"

// Страница → режим lv_keyboard (в порядке enum Page).
// Страницы символов делят TEXT_UPPER: карту подставляет _set_sym_map().
static const lv_keyboard_mode_t PAGE_MODE[] = {
    LV_KEYBOARD_MODE_USER_1, LV_KEYBOARD_MODE_USER_2,
    LV_KEYBOARD_MODE_USER_3, LV_KEYBOARD_MODE_USER_4,
    LV_KEYBOARD_MODE_SPECIAL, LV_KEYBOARD_MODE_NUMBER,
    LV_KEYBOARD_MODE_TEXT_LOWER,
    LV_KEYBOARD_MODE_TEXT_UPPER, LV_KEYBOARD_MODE_TEXT_UPPER };

// ── attach ────────────────────────────────────────────────
void Keyboard_Class::attach(lv_obj_t* parent, lv_obj_t* field, KbType type) {
    if (_kb) return;

    _kb = lv_keyboard_create(parent);
    // Штатный обработчик нам не нужен: он перехватывает подписи abc/ABC/1# и
    // LV_SYMBOL_CLOSE/KEYBOARD/OK (переключает режим, шлёт CANCEL/READY). Всё разбираем сами.
    lv_obj_remove_event_cb(_kb, lv_keyboard_def_event_cb);
    lv_obj_set_size(_kb, PDA2_SCREEN_W, PDA2_KB_H);
    lv_obj_align(_kb, LV_ALIGN_TOP_LEFT, 0, PDA2_KB_Y);
    lv_keyboard_set_popovers(_kb, false);

    _apply_layout();   // карты страниц и внешний вид клавиш — Keyboard_Layout.cpp

    _head = 0;
    _count = 0;
    _nbinds = 0;
    _field = nullptr;
    _long_hint = nullptr;
    _type = type;
    _make_hints();      // до _start_page: _show() синхронизирует видимость подсказок
    _make_popup();      // тоже до _start_page: _show() прячет попап
    _start_page(type);

    lv_obj_add_event_cb(_kb, _touch_cb,  LV_EVENT_PRESSED,       this);
    lv_obj_add_event_cb(_kb, _touch_cb,  LV_EVENT_LONG_PRESSED,  this);
    lv_obj_add_event_cb(_kb, _touch_cb,  LV_EVENT_RELEASED,      this);
    lv_obj_add_event_cb(_kb, _touch_cb,  LV_EVENT_PRESS_LOST,    this);
    lv_obj_add_event_cb(_kb, _event_cb,  LV_EVENT_VALUE_CHANGED, this);
    lv_obj_add_event_cb(_kb, _delete_cb, LV_EVENT_DELETE,        this);
    lv_obj_add_event_cb(_kb, _touch_cb,  LV_EVENT_PRESSING,      this);

    // Тап вне клавиатуры (по свободному месту экрана) — скрыть;
    // тап по полю — показать. Колбэки живут на объектах экрана и
    // уходят вместе с ним.
    if (parent) lv_obj_add_event_cb(parent, _outside_cb, LV_EVENT_CLICKED, this);

    if (field) {
        bind(field, type);
        _activate(_binds[0]);   // первое поле активно сразу
    }
}

// ── bind: ещё одно поле и его тип ─────────────────────────
void Keyboard_Class::bind(lv_obj_t* field, KbType type) {
    if (!_kb || !field || _nbinds >= PDA2_KB_FIELDS) return;

    Bind& b = _binds[_nbinds++];
    b.self = this;
    b.obj  = field;
    b.type = type;

    if (type == KBT_PASSWORD) lv_textarea_set_password_mode(field, true);
    lv_obj_add_event_cb(field, _field_cb, LV_EVENT_CLICKED, &b);
}

// ── стартовая страница по типу ────────────────────────────
void Keyboard_Class::_start_page(KbType type) {
    switch (type) {
        case KBT_PASSWORD: _lang_en = true;  _show(P_EN_LO); break;
        case KBT_NUMBER:                     _show(P_PAD);   break;
        default:           _lang_en = false; _show(P_RU_LO); break;
    }
}

// ── активация поля: тип, страница, фокус, показ ───────────
void Keyboard_Class::_activate(Bind& b) {
    if (!_kb) return;

    if (_field != b.obj) {
        if (_field) lv_obj_remove_state(_field, LV_STATE_FOCUSED);
        _field = b.obj;
        lv_obj_add_state(_field, LV_STATE_FOCUSED);
        _head = 0;     // события прежнего поля не переносим
        _count = 0;
    }
    if (_type != b.type) {
        _type = b.type;
        _start_page(_type);
    }
    show();
}

// ── show / hide ───────────────────────────────────────────
void Keyboard_Class::show() {
    if (!_kb) return;
    lv_obj_remove_flag(_kb, LV_OBJ_FLAG_HIDDEN);
}

void Keyboard_Class::hide() {
    if (!_kb) return;
    lv_obj_add_flag(_kb, LV_OBJ_FLAG_HIDDEN);
    _head  = 0;     // хвост событий не нужен
    _count = 0;
    _long_hint = nullptr;
    _popup_hide();
    _bs_hold = false;
}

// ── get ───────────────────────────────────────────────────
bool Keyboard_Class::get(KbEvent& ev) {
    if (_hide_pending && (int32_t)(pda2_platform_now_ms() - _hide_at) >= 0) {
        _panel_hide();
        _hide_pending = false;
    }
    if (_bs_hold && _count == 0) {
        const uint32_t now = pda2_platform_now_ms();
        if ((int32_t)(now - _bs_next) >= 0) {
            _push(KB_BACKSPACE, "");
            _bs_next = now + PDA2_KB_BS_MS;
        }
    }
    if (_count == 0) return false;
    ev = _q[_head];
    _head = (_head + 1) % PDA2_KB_QUEUE;
    _count--;
    return true;
}

// ── очередь ───────────────────────────────────────────────
void Keyboard_Class::_push(KbEventType t, const char* utf8) {
    if (_count >= PDA2_KB_QUEUE) return;   // переполнение — событие теряется
    KbEvent& e = _q[(_head + _count) % PDA2_KB_QUEUE];
    e.type = t;
    strncpy(e.utf8, utf8, 4);
    e.utf8[4] = '\0';
    _count++;
}

// ── смена страницы ────────────────────────────────────────
void Keyboard_Class::_show(Page p) {
    _page = p;
    if (p == P_SYM_RU || p == P_SYM_EN) _set_sym_map(p);   // карта символов в общий режим
    lv_keyboard_set_mode(_kb, PAGE_MODE[p]);
    _sync_hints();
    _popup_hide();
}

// ── разбор нажатой клавиши (по тексту) ────────────────────
void Keyboard_Class::_handle(const char* txt) {
    const bool upper = (_page == P_RU_UP || _page == P_EN_UP);
    const bool num   = (_page == P_NUM_RU || _page == P_NUM_EN);
    const bool sym   = (_page == P_SYM_RU || _page == P_SYM_EN);
    const Page lower = _lang_en ? P_EN_LO : P_RU_LO;

    if (!strcmp(txt, " ")) return;   // скрытая прокладка

    if (!strcmp(txt, KB_KEY_SHIFT) || !strcmp(txt, KB_KEY_SHIFT_UP)) {
        if (_lang_en) _show(upper ? P_EN_LO : P_EN_UP);
        else          _show(upper ? P_RU_LO : P_RU_UP);
        return;
    }
    if (!strcmp(txt, KB_KEY_BS))    { _push(KB_BACKSPACE, ""); return; }
    if (!strcmp(txt, KB_KEY_ENTER)) { _push(KB_ENTER, "");     return; }
    if (!strcmp(txt, KB_KEY_NUM)) { _show(_lang_en ? P_NUM_EN : P_NUM_RU); return; }
    if (!strcmp(txt, KB_KEY_SYM)) { _show(_lang_en ? P_SYM_EN : P_SYM_RU); return; }
    if (!strcmp(txt, KB_KEY_ABC_RU) || !strcmp(txt, KB_KEY_ABC_EN)) { _show(lower); return; }
    if (!strcmp(txt, "EN") || !strcmp(txt, "RU")) {
        _lang_en = !strcmp(txt, "EN");   // подпись = язык, на который переключаемся
        if (num)      _show(_lang_en ? P_NUM_EN : P_NUM_RU);
        else if (sym) _show(_lang_en ? P_SYM_EN : P_SYM_RU);
        else          _show(_lang_en ? P_EN_LO  : P_RU_LO);
        return;
    }
    if (!strcmp(txt, "Русский") || !strcmp(txt, "English")) {
        _push(KB_CHAR, " ");
        return;
    }

    _push(KB_CHAR, txt);
    if (upper) _show(lower);   // ⇧ одноразовый
}

// ── колбэки LVGL ──────────────────────────────────────────
void Keyboard_Class::_event_cb(lv_event_t* e) {
    Keyboard_Class* self = (Keyboard_Class*)lv_event_get_user_data(e);
    lv_obj_t* obj = (lv_obj_t*)lv_event_get_current_target(e);

    uint32_t id = lv_buttonmatrix_get_selected_button(obj);
    if (id == LV_BUTTONMATRIX_BUTTON_NONE) return;
    const char* txt = lv_buttonmatrix_get_button_text(obj, id);
    if (txt == NULL) return;
    if (self->_bs_hold && !strcmp(txt, KB_KEY_BS)) return;   // повтор ⌫ ведёт get()
    // Долгое нажатие запомнило подсказку: на отпускании вводим её вместо буквы,
    // если отпустили на той же клавише. Запомненное расходуется в любом случае.
    if (self->_long_hint) {
        
        const char* hint = self->_long_hint;
        const bool  same = (id == self->_long_id);
        self->_long_hint = nullptr;
        if (same) { self->_push(KB_CHAR, hint); return; }
    }
    if (self->_var_on) return;   // 3в: ввод варианта идёт на RELEASED
    
    self->_handle(txt);
}

void Keyboard_Class::_outside_cb(lv_event_t* e) {
    // Только клик по самому экрану: клик, всплывший от дочернего объекта, не считается.
    if (lv_event_get_target(e) != lv_event_get_current_target(e)) return;
    Keyboard_Class* self = (Keyboard_Class*)lv_event_get_user_data(e);
    self->hide();
}

void Keyboard_Class::_field_cb(lv_event_t* e) {
    Bind* b = (Bind*)lv_event_get_user_data(e);
    b->self->_activate(*b);
}

void Keyboard_Class::_delete_cb(lv_event_t* e) {
    Keyboard_Class* self = (Keyboard_Class*)lv_event_get_user_data(e);
    self->_kb        = nullptr;
    self->_field     = nullptr;
    self->_hints     = nullptr;
    self->_long_hint = nullptr;
    self->_nbinds    = 0;
    self->_head      = 0;
    self->_count     = 0;
    self->_cpanel = nullptr; self->_cleg = nullptr;
    for (int i = 0; i < 2; i++) { self->_cell[i] = nullptr; self->_cell_lbl[i] = nullptr; }
    self->_var_on = false;
    self->_bs_hold = false;
    self->_pop_id = LV_BUTTONMATRIX_BUTTON_NONE;
    self->_hide_pending = false;
}