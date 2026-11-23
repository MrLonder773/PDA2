// ════════════════════════════════════════════════════════
//  PDA 2 — KeyboardTestApp.cpp
//  Правила: lv_timer запрещены; ритм через onTick().
//  Экран ленивый: строится в onOpen(), удаляется Apps_Class.
// ════════════════════════════════════════════════════════

#include "KeyboardTestApp.h"

void KeyboardTestApp::onInit() {
    // нет состояния для инициализации здесь — всё уже задано
    // дефолтными значениями членов в .h и сбрасывается в onOpen()
}

// Подпись (статичная, фон прозрачный) + поле. Возвращает поле.
lv_obj_t* KeyboardTestApp::_make_field(int y, const char* caption) {
    lv_obj_t* lbl = lv_label_create(screen);
    lv_label_set_text(lbl, caption);
    lv_obj_set_pos(lbl, 8, y);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xD9D9D9), 0);
    lv_obj_set_style_text_font(lbl, PDA.Fonts.get(12), 0);

    lv_obj_t* ta = lv_textarea_create(screen);
    lv_obj_set_size(ta, 304, 48);
    lv_obj_set_pos(ta, 8, y + 16);
    lv_textarea_set_one_line(ta, true);
    lv_obj_set_style_bg_color(ta, lv_color_hex(0x23304A), 0);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ta, 1, 0);
    lv_obj_set_style_border_color(ta, lv_color_hex(0x4A5A7C), 0);
    lv_obj_set_style_text_color(ta, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(ta, PDA.Fonts.get(16), 0);

    // Курсор — тонкая линия слева, виден только в активном (FOCUSED) поле.
    // Стиль задан и для обычного, и для FOCUSED-состояния: тема красит FOCUSED сама.
    const lv_style_selector_t cur   = (lv_style_selector_t)LV_PART_CURSOR;
    const lv_style_selector_t cur_f = cur | (lv_style_selector_t)LV_STATE_FOCUSED;
    lv_obj_set_style_bg_opa(ta, LV_OPA_TRANSP, cur);
    lv_obj_set_style_bg_opa(ta, LV_OPA_TRANSP, cur_f);
    lv_obj_set_style_border_side(ta, LV_BORDER_SIDE_LEFT, cur);
    lv_obj_set_style_border_side(ta, LV_BORDER_SIDE_LEFT, cur_f);
    lv_obj_set_style_border_width(ta, 2, cur);
    lv_obj_set_style_border_width(ta, 2, cur_f);
    lv_obj_set_style_border_color(ta, lv_color_hex(0xffffff), cur);
    lv_obj_set_style_border_color(ta, lv_color_hex(0xffffff), cur_f);
    lv_obj_set_style_border_opa(ta, LV_OPA_TRANSP, cur);     // не в фокусе — не виден
    lv_obj_set_style_border_opa(ta, LV_OPA_COVER,  cur_f);   // в фокусе — виден
    return ta;
}

void KeyboardTestApp::onOpen() {
    if (screen) return;

    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x1a2438), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    _ta  = _make_field(32,  "Текст");
    _pw  = _make_field(104, "Пароль");
    _num = _make_field(176, "Число");

    // Первое поле активно сразу (текст); остальные — по тапу.
    PDA.Keyboard.attach(screen, _ta, KBT_TEXT);
    PDA.Keyboard.bind(_pw,  KBT_PASSWORD);
    PDA.Keyboard.bind(_num, KBT_NUMBER);
}

void KeyboardTestApp::onClose() {
    // экран удалится сам; указатели на поля больше недействительны
    _ta = _pw = _num = nullptr;
}

void KeyboardTestApp::onTick(uint32_t delta_ms) {
    if (!_ta) return;

    lv_obj_t* f = PDA.Keyboard.field();   // активное поле

    KbEvent ev;
    while (PDA.Keyboard.get(ev)) {
        if (!f) continue;                 // поля нет — событие просто сбрасываем
        switch (ev.type) {
            case KB_CHAR:      lv_textarea_add_text(f, ev.utf8); break;
            case KB_BACKSPACE: lv_textarea_delete_char(f);       break;
            case KB_ENTER:     PDA.Keyboard.hide();              break;  // однострочные поля
            
        }
    }
}