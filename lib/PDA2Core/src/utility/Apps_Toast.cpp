// ════════════════════════════════════════════════════════
//  PDA 2 — Apps_Toast.cpp
//  Тост поверх всего (lv_layer_top).
// ════════════════════════════════════════════════════════

#include <PDA2.h>
#include "Apps_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"
#include "../pda2_platform.h"

extern const lv_font_t pda2_cyrillic_16;

void Apps_Class::_build_toast() {
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

void Apps_Class::showToast(const char* text, uint32_t ms) {
    if (!_toast_cont || !_toast_label) return;
    lv_label_set_text(_toast_label, text);
    lv_obj_clear_flag(_toast_cont, LV_OBJ_FLAG_HIDDEN);
    _toast_until = pda2_platform_now_ms() + ms;
    PDA_LOGI("apps", "Toast: %s (%ums)", text, ms);
}

void Apps_Class::_tick_toast(uint32_t now) {
    if (!_toast_cont) return;
    if (lv_obj_has_flag(_toast_cont, LV_OBJ_FLAG_HIDDEN)) return;
    if (now >= _toast_until) {
        lv_obj_add_flag(_toast_cont, LV_OBJ_FLAG_HIDDEN);
    }
}