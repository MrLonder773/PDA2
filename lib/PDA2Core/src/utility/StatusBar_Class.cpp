#include "StatusBar_Class.h"
#include <PDA2.h>
#include "../pda2_platform.h"
#include <stdio.h>

// Иконки (lib/PDA2Core/src/icons/*.c, формат RGB565A8)
extern "C" {
extern const lv_image_dsc_t icon_battery;
extern const lv_image_dsc_t icon_bt;
extern const lv_image_dsc_t icon_wifi_0;
extern const lv_image_dsc_t icon_wifi_1;
extern const lv_image_dsc_t icon_wifi_2;
extern const lv_image_dsc_t icon_wifi_3;
extern const lv_image_dsc_t icon_wifi_4;
}

// ═══════════════════════════════════════════════════════════
//  Геометрия (Figma). Иконки ставятся по ЦЕНТРУ блока Figma —
//  массивы чуть больше блоков (гипотеза, сверить на экране).
//  *_CX2/_CY2 — центр блока ×2, чтобы округлять целочисленно.
// ═══════════════════════════════════════════════════════════

static constexpr int32_t BAR_H     = 27;

static constexpr int32_t TIME_X    = 19;
static constexpr int32_t BLOCK_Y   = 4;    // верх блоков
static constexpr int32_t BLOCK_H   = 20;

static constexpr int32_t BT_CX2    = 411;  // x198, 15 → центр 205.5
static constexpr int32_t WIFI_CX2  = 470;  // x221, 28.46 → центр 235.23
static constexpr int32_t BATT_CX2  = 559;  // x257, 45 → центр 279.5
static constexpr int32_t ICON_CY2  = 28;   // y4, h20 → центр 14

static constexpr int32_t FILL_X      = 260;
static constexpr int32_t FILL_Y      = 9;
static constexpr int32_t FILL_W      = 32;   // ширина при 100%
static constexpr int32_t FILL_H      = 11;
static constexpr int32_t FILL_RADIUS = 0;          // Figma: corner radius 0

static constexpr lv_opa_t OPA_FILL   = 140;        // Figma: 55% от 255
static constexpr uint32_t COLOR_FILL = 0x4911CC;   // Figma: #4911CC

static constexpr uint32_t TIME_POLL_MS = 1000;   // Rtc.get() читает шину — не чаще 1 раза в сек

// ═══════════════════════════════════════════════════════════
//  Помощники
// ═══════════════════════════════════════════════════════════

static void make_inert(lv_obj_t* o) {
    lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
}

static void place_centered(lv_obj_t* img, const lv_image_dsc_t* d, int32_t cx2) {
    lv_obj_set_pos(img, (cx2 - (int32_t)d->header.w) / 2,
                        (ICON_CY2 - (int32_t)d->header.h) / 2);
}

// Метки: прозрачный фон (исключение из правила 9, см. доку).
// y — по центру блока высотой BLOCK_H с учётом высоты строки шрифта.
static lv_obj_t* make_label(lv_obj_t* parent, int32_t x) {
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_style_text_color(l, lv_color_hex(UI_COLOR_TEXT), 0);
    lv_obj_set_style_text_font(l, PDA.Fonts.get(16), 0);
    const int32_t lh = lv_font_get_line_height(PDA.Fonts.get(16));
    lv_obj_set_pos(l, x, BLOCK_Y + (BLOCK_H - lh) / 2);
    make_inert(l);
    return l;
}

// ═══════════════════════════════════════════════════════════
//  begin
// ═══════════════════════════════════════════════════════════

void StatusBar_Class::begin() {
    _bar = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_bar, PDA2_SCREEN_W, BAR_H);
    lv_obj_set_pos(_bar, 0, 0);
    lv_obj_set_style_bg_opa(_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_bar, 0, 0);
    lv_obj_set_style_radius(_bar, 0, 0);
    lv_obj_set_style_pad_all(_bar, 0, 0);
    make_inert(_bar);

    _lbl_time = make_label(_bar, TIME_X);
    lv_label_set_text(_lbl_time, "--:--");

    // Порядок важен: заливка → контур → число (снизу вверх)
    _fill = lv_obj_create(_bar);
    lv_obj_set_pos(_fill, FILL_X, FILL_Y);
    lv_obj_set_size(_fill, FILL_W, FILL_H);
    lv_obj_set_style_bg_color(_fill, lv_color_hex(COLOR_FILL), 0);
    lv_obj_set_style_bg_opa(_fill, OPA_FILL, 0);       // было LV_OPA_COVER
    lv_obj_set_style_border_width(_fill, 0, 0);
    lv_obj_set_style_radius(_fill, FILL_RADIUS, 0);
    lv_obj_set_style_pad_all(_fill, 0, 0);
    make_inert(_fill);
    lv_obj_add_flag(_fill, LV_OBJ_FLAG_HIDDEN);

    _img_battery = lv_image_create(_bar);
    lv_image_set_src(_img_battery, &icon_battery);
    place_centered(_img_battery, &icon_battery, BATT_CX2);
    make_inert(_img_battery);

    // Число заряда: ширина = заливка, текст по центру (без «%»)
    _lbl_battery = make_label(_bar, FILL_X);
    lv_obj_set_width(_lbl_battery, FILL_W);
    lv_obj_set_style_text_align(_lbl_battery, LV_TEXT_ALIGN_CENTER, 0);

    _img_bt = lv_image_create(_bar);
    lv_image_set_src(_img_bt, &icon_bt);
    place_centered(_img_bt, &icon_bt, BT_CX2);
    make_inert(_img_bt);
    lv_obj_add_flag(_img_bt, LV_OBJ_FLAG_HIDDEN);

    _img_wifi = lv_image_create(_bar);
    lv_image_set_src(_img_wifi, &icon_wifi_0);
    place_centered(_img_wifi, &icon_wifi_0, WIFI_CX2);
    make_inert(_img_wifi);
    lv_obj_add_flag(_img_wifi, LV_OBJ_FLAG_HIDDEN);

    if (!PDA.Prefs.getStatusBarVisible()) lv_obj_add_flag(_bar, LV_OBJ_FLAG_HIDDEN);
    _apply_battery();
    PDA_LOGI("statusbar", "StatusBar ready");
}

// ═══════════════════════════════════════════════════════════
//  update
// ═══════════════════════════════════════════════════════════

void StatusBar_Class::update() {
    if (!_bar) return;

    _update_time();

    // Bluetooth: виден, пока стек не OFF
    const int8_t bt = (PDA.Bt.state() != BT_OFF) ? 1 : 0;
    if (bt != _bt_vis) {
        _bt_vis = bt;
        if (bt) lv_obj_clear_flag(_img_bt, LV_OBJ_FLAG_HIDDEN);
        else    lv_obj_add_flag(_img_bt, LV_OBJ_FLAG_HIDDEN);
    }

    // Wi-Fi: уровня сигнала у Wifi_Class нет — по состоянию
    int8_t lvl = -1;
    switch (PDA.Wifi.state()) {
        case WifiState::STA_CONNECTING:
        case WifiState::STA_FAILED:    lvl = 0; break;
        case WifiState::STA_CONNECTED: lvl = 4; break;   // заглушка до RSSI
        case WifiState::OFF:
        case WifiState::AP_ACTIVE:     lvl = -1; break;
    }
    if (lvl != _wifi_lvl) {
        _wifi_lvl = lvl;
        if (lvl < 0) {
            lv_obj_add_flag(_img_wifi, LV_OBJ_FLAG_HIDDEN);
        } else {
            static const lv_image_dsc_t* const icons[5] = {
                &icon_wifi_0, &icon_wifi_1, &icon_wifi_2, &icon_wifi_3, &icon_wifi_4
            };
            lv_image_set_src(_img_wifi, icons[lvl]);
            place_centered(_img_wifi, icons[lvl], WIFI_CX2);   // ширина иконок разная (28/29)
            lv_obj_clear_flag(_img_wifi, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void StatusBar_Class::_update_time() {
    const uint32_t now = pda2_platform_now_ms();
    if (_time_init && (now - _time_ms) < TIME_POLL_MS) return;
    _time_init = true;
    _time_ms   = now;

    // Нет чипа — шину не трогаем (иначе ошибка I2C в лог каждую секунду)
    int16_t key = -1;
    pda2_time_t t = {};
    if (PDA.Rtc.ok()) {
        t = PDA.Rtc.get();
        if (t.valid) key = (int16_t)(t.hour * 60 + t.minute);
    }
    if (key == _time_key) return;
    _time_key = key;

    if (key < 0) {
        lv_label_set_text(_lbl_time, "--:--");
    } else {
        char buf[8];
        snprintf(buf, sizeof(buf), "%02d:%02d", t.hour, t.minute);
        lv_label_set_text(_lbl_time, buf);
    }
}

// ═══════════════════════════════════════════════════════════
//  Батарея
// ═══════════════════════════════════════════════════════════

void StatusBar_Class::setBattery(int percent) {
    if (percent > 100) percent = 100;
    if (percent < 0)   percent = -1;
    if (percent == _battery) return;
    _battery = percent;
    if (_bar) _apply_battery();
}

void StatusBar_Class::_apply_battery() {
    if (_battery < 0) {
        lv_obj_add_flag(_fill, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(_lbl_battery, "--");
        return;
    }
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", _battery);
    lv_label_set_text(_lbl_battery, buf);

    const int32_t w = FILL_W * _battery / 100;
    if (w <= 0) {
        lv_obj_add_flag(_fill, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_set_width(_fill, w);
        lv_obj_clear_flag(_fill, LV_OBJ_FLAG_HIDDEN);
    }
}

void StatusBar_Class::toggle() {
    if (!_bar) return;
    const bool show = lv_obj_has_flag(_bar, LV_OBJ_FLAG_HIDDEN);
    if (show) lv_obj_clear_flag(_bar, LV_OBJ_FLAG_HIDDEN);
    else      lv_obj_add_flag(_bar, LV_OBJ_FLAG_HIDDEN);
    PDA.Prefs.setStatusBarVisible(show);
}