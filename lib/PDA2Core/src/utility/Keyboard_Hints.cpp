// ════════════════════════════════════════════════════════
//  PDA 2 — Keyboard_Hints.cpp (блок класса Keyboard_Class)
//  lib/PDA2Core/src/utility/Keyboard_Hints.cpp
//
//  Шаг 3а: подсказки RU (нижний регистр) — таблица HINTS_RU,
//  метки в углу клавиш, долгое нажатие вводит подсказку.
//  Шаг 3б: попап над клавишей на долгом нажатии.
// ════════════════════════════════════════════════════════

#include "Keyboard_Class.h"
#include "../PDA2.h"
#include <string.h>
#include "../pda2_platform.h"

// ── Подсказки RU, нижний регистр (шаг 3а) ─────────────────
// key — текст клавиши в карте, hint — вводится на долгом нажатии,
// row/col — положение клавиши (все клавиши RU шириной 1, 11 колонок).
// Клавиши без строки в таблице (в т.ч. «х») подсказки не имеют.
struct HintDef { const char* key; const char* hint; uint8_t row; uint8_t col; const char* alt; };
static const HintDef HINTS_RU[] = {
    {"й","1",0,0,nullptr}, {"ц","2",0,1,nullptr}, {"у","3",0,2,nullptr}, {"к","4",0,3,nullptr},
    {"е","5",0,4,"ё"},
    {"н","6",0,5,nullptr}, {"г","7",0,6,nullptr}, {"ш","8",0,7,nullptr}, {"щ","9",0,8,nullptr},
    {"з","0",0,9,nullptr}, {"ь","ъ",2,7,nullptr} };
static const uint8_t N_HINTS_RU = sizeof(HINTS_RU) / sizeof(HINTS_RU[0]);

// Клавиши без попапа на нажатии: служебные и пробел (подписи как в картах).
static bool is_char_key(const char* t) {
    static const char* const ctl[] = {KB_KEY_SHIFT,KB_KEY_SHIFT_UP,KB_KEY_BS,KB_KEY_ENTER,KB_KEY_NUM,KB_KEY_SYM,KB_KEY_ABC_RU,KB_KEY_ABC_EN,"EN","RU"," ","Русский","English"};
    for (uint8_t i = 0; i < sizeof(ctl) / sizeof(ctl[0]); i++) if (!strcmp(t, ctl[i])) return false;
    return true;
}

// ── подсказки: прозрачный контейнер с мелкими метками ─────
// Геометрия считается от PDA2_SCREEN_W / PDA2_KB_H / PDA2_KB_GAP:
// ширина клавиши RU = (W - 10*gap)/11, шаг колонки = (W + gap)/11,
// шаг ряда = (H + gap)/4. Подгонка — на глаз в sim.
void Keyboard_Class::_make_hints() {
    const int W = PDA2_SCREEN_W;
    const int H = PDA2_KB_H;
    const int G = PDA2_KB_GAP;
    const int cell_w = (W - 10 * G) / 11;

    _hints = lv_obj_create(_kb);
    lv_obj_set_size(_hints, W, H);
    lv_obj_set_pos(_hints, 0, 0);
    lv_obj_set_style_bg_opa(_hints, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_hints, 0, 0);
    lv_obj_set_style_pad_all(_hints, 0, 0);
    lv_obj_remove_flag(_hints, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(_hints, LV_OBJ_FLAG_SCROLLABLE);

    for (uint8_t i = 0; i < N_HINTS_RU; i++) {
        const HintDef& h = HINTS_RU[i];
        lv_obj_t* l = lv_label_create(_hints);
        lv_label_set_text(l, h.hint);
        lv_obj_set_style_text_font(l, PDA.Fonts.get(12), 0);
        lv_obj_set_style_text_color(l, lv_color_hex(0x8FA0C0), 0);
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_width(l, cell_w - 3);
        lv_obj_set_pos(l, (h.col * (W + G)) / 11, (h.row * (H + G)) / 4 + 1);
        lv_obj_remove_flag(l, LV_OBJ_FLAG_CLICKABLE);
    }
}

// Подсказки видны только на RU, нижний регистр.
void Keyboard_Class::_sync_hints() {
    if (!_hints) return;
    if (_page == P_RU_LO) lv_obj_remove_flag(_hints, LV_OBJ_FLAG_HIDDEN);
    else                  lv_obj_add_flag(_hints, LV_OBJ_FLAG_HIDDEN);
}

// ── попап (шаг 3б): синий прямоугольник над нажатой клавишей ──
// Дочерний родителя клавиатуры, а не самой клавиатуры: внутри неё
// попап верхнего ряда обрезался бы границей виджета. Размер и место —
// от PDA2_KB_* и row/col клавиши: ширина клавиши + 8 px, высота две
// высоты клавиши, нижний край на нижнем крае клавиши (накрывает её).
// Подгонка шрифта и отступа текста — на глаз в sim.
void Keyboard_Class::_make_popup() {
    lv_obj_t* parent = lv_obj_get_parent(_kb);
    _cpanel = lv_obj_create(parent);
    _cleg   = lv_obj_create(parent);

    lv_obj_t* bg[2] = {_cpanel, _cleg};
    for (int i = 0; i < 2; i++) {
        lv_obj_remove_flag(bg[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(bg[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(bg[i], lv_color_hex(0x3A4252), 0);   // временно литералом
        lv_obj_set_style_bg_opa(bg[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(bg[i], i == 0 ? 8 : 4, 0);
        lv_obj_set_style_border_width(bg[i], 0, 0);
        lv_obj_set_style_shadow_width(bg[i], 0, 0);
        lv_obj_set_style_pad_all(bg[i], 0, 0);
        lv_obj_add_flag(bg[i], LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = 0; i < 2; i++) {
        _cell[i] = lv_obj_create(_cpanel);
        lv_obj_remove_flag(_cell[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(_cell[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(_cell[i], lv_color_hex(0x4A7BD0), 0);
        lv_obj_set_style_radius(_cell[i], 6, 0);
        lv_obj_set_style_border_width(_cell[i], 0, 0);
        lv_obj_set_style_shadow_width(_cell[i], 0, 0);
        lv_obj_set_style_pad_all(_cell[i], 0, 0);
        _cell_lbl[i] = lv_label_create(_cell[i]);
        lv_obj_set_style_text_font(_cell_lbl[i], PDA.Fonts.get(24), 0);
        lv_obj_set_style_text_color(_cell_lbl[i], lv_color_hex(0xffffff), 0);
        lv_obj_center(_cell_lbl[i]);
    }
}

// Одиночная подсказка: та же панель с «ножкой», одна синяя ячейка.
// _var_on не ставится: ввод идёт через _long_hint на VALUE, как в 3а.
void Keyboard_Class::_popup_show(uint32_t id, const char* txt) {
    _var[0] = txt; _var_n = 1; _var_idx = 0;
    _cells_show(id);
}

void Keyboard_Class::_panel_hide() {
    if (_cpanel) lv_obj_add_flag(_cpanel, LV_OBJ_FLAG_HIDDEN);
    if (_cleg)   lv_obj_add_flag(_cleg,   LV_OBJ_FLAG_HIDDEN);
}

void Keyboard_Class::_popup_hide() {
    _panel_hide();
    _pop_id = LV_BUTTONMATRIX_BUTTON_NONE;
    _var_on = false;
    _hide_pending = false;
}

void Keyboard_Class::_popup_release() {
    _var_on = false;
    _pop_id = LV_BUTTONMATRIX_BUTTON_NONE;
    if (PDA2_KB_POP_HIDE_MS == 0) { _panel_hide(); return; }
    _hide_at = pda2_platform_now_ms() + PDA2_KB_POP_HIDE_MS;
    _hide_pending = true;
}

// Попап для клавиши под пальцем: символьная — панель с её буквой, служебная — без попапа.
void Keyboard_Class::_press_popup(lv_obj_t* obj) {
    const uint32_t id = lv_buttonmatrix_get_selected_button(obj);
    if (id == LV_BUTTONMATRIX_BUTTON_NONE || id == _pop_id) return;  
    // Первый попап за касание ждёт PDA2_KB_POP_MS; после показа попап переезжает сразу.
    if (_pop_id == LV_BUTTONMATRIX_BUTTON_NONE &&
    pda2_platform_now_ms() - _press_ms < PDA2_KB_POP_MS) return;
    _pop_id = id;
    const char* txt = lv_buttonmatrix_get_button_text(obj, id);
    if (!txt || !is_char_key(txt)) { _panel_hide(); return; }
    _popup_show(id, txt);
}

// Панель над рядом (нижний край на 4 px выше ряда), ячейка 36 px, первая над клавишей;
// «ножка» шириной в клавишу накрывает её (буква скрыта).
void Keyboard_Class::_cells_show(uint32_t id) {
    if (!_cpanel || !_cleg) return;
    int key_x, key_y, key_w, key_h;
    if (!_key_rect(id, key_x, key_y, key_w, key_h)) return;
    const int W = PDA2_SCREEN_W;
    const int CW = 36, PAD = 4;
    const int pw = _var_n * CW + 2 * PAD;
    const int ph = key_h + 8;
    _var_cx = key_x + key_w / 2;

    int x0 = _var_cx - CW / 2 - PAD;
    if (x0 > W - pw) x0 = W - pw;
    if (x0 < 0)      x0 = 0;
    const int py = key_y - 4 - ph;

    lv_obj_set_size(_cpanel, pw, ph);
    lv_obj_set_pos(_cpanel, x0, py);
    const int ly = py + ph - 2;
    lv_obj_set_size(_cleg, key_w, key_y + key_h - ly);
    lv_obj_set_pos(_cleg, key_x, ly);

    for (uint8_t i = 0; i < 2; i++) {
        if (i < _var_n) {
            lv_obj_set_size(_cell[i], CW, ph - 8);
            lv_obj_set_pos(_cell[i], PAD + i * CW, 4);
            lv_label_set_text(_cell_lbl[i], _var[i]);
            lv_obj_center(_cell_lbl[i]);
            lv_obj_remove_flag(_cell[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(_cell[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    lv_obj_remove_flag(_cpanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(_cleg,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(_cleg);
    lv_obj_move_foreground(_cpanel);
    _cells_update(_var_cx);
}

// Индекс = смещение пальца по x от центра клавиши / ширина ячейки, с упором в крайние.
void Keyboard_Class::_cells_update(int x) {
    const int CW = 36;
    int d = x - _var_cx + CW / 2;
    int idx = d < 0 ? 0 : d / CW;
    if (idx > _var_n - 1) idx = _var_n - 1;
    _var_idx = (uint8_t)idx;
    for (uint8_t i = 0; i < _var_n; i++)
        lv_obj_set_style_bg_opa(_cell[i], i == _var_idx ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
}

// PRESSED — сбросить запомненное, спрятать попап; RELEASED/PRESS_LOST — спрятать попап
// (PRESS_LOST ещё и сбрасывает подсказку: палец ушёл с клавиши, подсказка не вводится);
// LONG_PRESSED — запомнить подсказку клавиши и показать попап (только RU, нижний регистр;
// клавиша без строки в HINTS_RU ничего не запоминает и попап не показывает).
void Keyboard_Class::_touch_cb(lv_event_t* e) {
    Keyboard_Class* self = (Keyboard_Class*)lv_event_get_user_data(e);
    const lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = (lv_obj_t*)lv_event_get_current_target(e);
    if (code == LV_EVENT_PRESSING) {
        if (self->_var_on) {
            lv_point_t p = {0, 0};
            lv_indev_t* di = lv_indev_active();
            if (di) lv_indev_get_point(di, &p);
            self->_cells_update(p.x);
        } else {
            self->_press_popup(obj);   // палец сдвинулся на другую клавишу — попап переезжает
        }
        return;
    }
    if (code == LV_EVENT_PRESSED) {
        self->_bs_hold = false; self->_long_hint = nullptr; self->_popup_hide();
        self->_press_ms = pda2_platform_now_ms();
        self->_press_popup(obj);
        return;
    }
    if (code == LV_EVENT_PRESS_LOST) {
        self->_bs_hold = false;
        if (self->_var_on) return;   // ряд вариантов не зависит от клавиши под пальцем
        self->_long_hint = nullptr; self->_popup_hide(); return;
    }
    if (code == LV_EVENT_RELEASED) {
        self->_bs_hold = false;
        if (self->_var_on) self->_push(KB_CHAR, self->_var[self->_var_idx]);
        self->_popup_release();
        return;
    }
    if (code != LV_EVENT_LONG_PRESSED) return;

    uint32_t id = lv_buttonmatrix_get_selected_button(obj);
    if (id == LV_BUTTONMATRIX_BUTTON_NONE) return;
    const char* txt = lv_buttonmatrix_get_button_text(obj, id);
    if (txt == NULL) return;

    if (!strcmp(txt, KB_KEY_BS)) {             // ⌫ на любой странице: свой автоповтор
        self->_bs_hold = true;
        self->_bs_next = pda2_platform_now_ms() + PDA2_KB_BS_MS;
        return;
    }
    if (self->_page != P_RU_LO) return;

    for (uint8_t i = 0; i < N_HINTS_RU; i++) {
        if (strcmp(txt, HINTS_RU[i].key)) continue;
        const HintDef& h = HINTS_RU[i];
        if (h.alt) {
            self->_long_hint = nullptr;
            self->_var[0] = h.hint; self->_var[1] = h.alt;
            self->_var_n = 2; self->_var_idx = 0;
            self->_var_on = true;
            self->_cells_show(id);
            return;
        }
        self->_long_hint = h.hint;
        self->_long_id   = id;
        self->_popup_show(id, h.hint);
        return;
    }
}