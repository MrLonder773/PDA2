// ════════════════════════════════════════════════════════
//  PDA 2 — ClockApp.cpp
//
//  Правила:
//    - lv_timer запрещены. Ритм через onTick(delta_ms).
//    - Все лейблы с динамическим текстом: bg_opa=COVER.
//    - RTC опрашивается раз в секунду через _tick_acc.
//    - Лейблы обновляются только при изменении значения.
// ════════════════════════════════════════════════════════

#include "ClockApp.h"

// ── Дни недели (DS3231: 1=Пн ... 7=Вс) ─────────────────
const char* ClockApp::_weekday_str(uint8_t wd) {
    switch (wd) {
        case 1: return "Monday";
        case 2: return "Tuesday";
        case 3: return "Wednesday";
        case 4: return "Thursday";
        case 5: return "Friday";
        case 6: return "Saturday";
        case 7: return "Sunday";
        default: return "---";
    }
}

// ── onInit — строим UI один раз ──────────────────────────
void ClockApp::onInit() {
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* cont = lv_obj_create(screen);
    lv_obj_set_size(cont, lv_pct(100), lv_pct(100));
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(cont, 16, 0);

    // ── Время HH:MM:SS — единый лейбл ───────────────────
    _lbl_time = lv_label_create(cont);
    lv_label_set_text(_lbl_time, "--:--:--");
    lv_obj_set_style_text_font(_lbl_time, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(_lbl_time, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_color(_lbl_time, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_bg_opa(_lbl_time, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(_lbl_time, 0, 0);

    // ── Дата ─────────────────────────────────────────────
    _lbl_date = lv_label_create(cont);
    lv_label_set_text(_lbl_date, "--- --.--.----");
    lv_obj_set_style_text_font(_lbl_date, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(_lbl_date, lv_color_hex(0xaaaacc), 0);
    lv_obj_set_style_bg_color(_lbl_date, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_bg_opa(_lbl_date, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(_lbl_date, 0, 0);
}

// ── onOpen ────────────────────────────────────────────────
void ClockApp::onOpen() {
    _prev_hh    = 0xFF;
    _prev_mm    = 0xFF;
    _prev_ss    = 0xFF;
    _prev_day   = 0xFF;   // [A-75]
    _prev_month = 0xFF;   // [A-75]
    _prev_year  = 0xFFFF; // [A-75]
    _tick_acc   = 1001;
}

// ── onClose ───────────────────────────────────────────────
void ClockApp::onClose() {}

// ── onTick ────────────────────────────────────────────────
void ClockApp::onTick(uint32_t delta_ms) {
    _tick_acc += delta_ms;
    if (_tick_acc < 1000) return;
    _tick_acc = 0;

    if (!PDA.Rtc.ok()) {
        lv_label_set_text(_lbl_time, "NO RTC");
        return;
    }

    pda2_time_t t = PDA.Rtc.get();
    _update_time(t);
    _update_date(t);
}

// ── _update_time ──────────────────────────────────────────
void ClockApp::_update_time(const pda2_time_t& t) {
    if (t.hour == _prev_hh && t.minute == _prev_mm && t.second == _prev_ss)
        return;

    _prev_hh = t.hour;
    _prev_mm = t.minute;
    _prev_ss = t.second;

    char buf[9];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.hour, t.minute, t.second);
    lv_label_set_text(_lbl_time, buf);
}

// ── _update_date ──────────────────────────────────────────
void ClockApp::_update_date(const pda2_time_t& t) {
    if (t.day == _prev_day && t.month == _prev_month && t.year == _prev_year) // [A-75]
        return;

    _prev_day   = t.day;   // [A-75]
    _prev_month = t.month; // [A-75]
    _prev_year  = t.year;  // [A-75]

    char buf[32];
    snprintf(buf, sizeof(buf), "%s, %02d.%02d.%04d",
             _weekday_str(t.weekday), t.day, t.month, t.year);
    lv_label_set_text(_lbl_date, buf);
}