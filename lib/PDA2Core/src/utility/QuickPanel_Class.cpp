#include "QuickPanel_Class.h"
#include <PDA2.h>
#include <WiFi.h>

// ═══════════════════════════════════════════════════════════
//  Константы панели
// ═══════════════════════════════════════════════════════════

static constexpr int32_t QP_NOTIF_H    = PDA2_QP_NOTIF_H;
static constexpr int32_t QP_SETTINGS_H = PDA2_QP_SETTINGS_H;
static constexpr uint32_t QP_ANIM_MS   = PDA2_QP_ANIM_MS;

static constexpr int32_t CARD_H        = 72;
static constexpr int32_t CARD_GAP      = 8;
static constexpr int32_t PANEL_PAD     = 10;

// ═══════════════════════════════════════════════════════════
//  begin
// ═══════════════════════════════════════════════════════════

void QuickPanel_Class::begin() {
    _build_overlay();
    _build_notif_panel();
    _build_settings_panel();
    PDA_LOGI("qpanel", "QuickPanel ready");
}

// ═══════════════════════════════════════════════════════════
//  _build_overlay
// ═══════════════════════════════════════════════════════════

void QuickPanel_Class::_build_overlay() {
    _overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_overlay, PDA2_SCREEN_W, PDA2_SCREEN_H);
    lv_obj_set_pos(_overlay, 0, 0);
    lv_obj_set_style_bg_color(_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(_overlay, LV_OPA_50, 0);
    lv_obj_set_style_border_width(_overlay, 0, 0);
    lv_obj_set_style_radius(_overlay, 0, 0);
    lv_obj_clear_flag(_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(_overlay, _cb_overlay_click, LV_EVENT_CLICKED, this);
}

// ═══════════════════════════════════════════════════════════
//  _build_notif_panel
// ═══════════════════════════════════════════════════════════

void QuickPanel_Class::_build_notif_panel() {
    _panel_notif = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_panel_notif, PDA2_SCREEN_W, QP_NOTIF_H);
    lv_obj_set_pos(_panel_notif, 0, -QP_NOTIF_H);
    lv_obj_set_style_bg_color(_panel_notif, lv_color_hex(0x111827), 0);
    lv_obj_set_style_bg_opa(_panel_notif, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_panel_notif, 0, 0);
    lv_obj_set_style_radius(_panel_notif, 0, 0);
    lv_obj_set_style_pad_all(_panel_notif, 0, 0);
    lv_obj_clear_flag(_panel_notif, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_panel_notif, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_user_data(_panel_notif, this);

    _cont_list = lv_obj_create(_panel_notif);
    lv_obj_set_size(_cont_list, PDA2_SCREEN_W, QP_NOTIF_H);
    lv_obj_set_pos(_cont_list, 0, 0);
    lv_obj_set_style_bg_opa(_cont_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_cont_list, 0, 0);
    lv_obj_set_style_pad_all(_cont_list, PANEL_PAD, 0);
    lv_obj_set_style_pad_row(_cont_list, CARD_GAP, 0);

    _lbl_empty = lv_label_create(_panel_notif);
    lv_label_set_text(_lbl_empty, "No notifications");
    lv_obj_set_style_text_color(_lbl_empty, lv_color_hex(0x4b5563), 0);
    lv_obj_set_style_text_font(_lbl_empty, &lv_font_montserrat_14, 0);
    lv_obj_set_style_bg_opa(_lbl_empty, LV_OPA_COVER, 0);   // TEAR-1
    lv_obj_set_style_bg_color(_lbl_empty, lv_color_hex(0x111827), 0);
    lv_obj_center(_lbl_empty);
    lv_obj_add_flag(_lbl_empty, LV_OBJ_FLAG_HIDDEN);
}

// ═══════════════════════════════════════════════════════════
//  _build_settings_panel
// ═══════════════════════════════════════════════════════════

void QuickPanel_Class::_build_settings_panel() {
    _panel_settings = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_panel_settings, PDA2_SCREEN_W, QP_SETTINGS_H);
    lv_obj_set_pos(_panel_settings, 0, -QP_SETTINGS_H);
    lv_obj_set_style_bg_color(_panel_settings, lv_color_hex(0x111827), 0);
    lv_obj_set_style_bg_opa(_panel_settings, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_panel_settings, 0, 0);
    lv_obj_set_style_radius(_panel_settings, 0, 0);
    lv_obj_set_style_pad_all(_panel_settings, 20, 0);
    lv_obj_clear_flag(_panel_settings, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_panel_settings, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_user_data(_panel_settings, this);

    // ── Яркость ─────────────────────────────────────────────
    lv_obj_t* lbl_br = lv_label_create(_panel_settings);
    lv_label_set_text(lbl_br, "Brightness");
    lv_obj_set_style_text_color(lbl_br, lv_color_hex(0x9ca3af), 0);
    lv_obj_set_style_text_font(lbl_br, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_br, 0, 16);

    _slider_br = lv_slider_create(_panel_settings);
    lv_obj_set_size(_slider_br, PDA2_SCREEN_W - 100, 20);
    lv_obj_set_pos(_slider_br, 0, 52);
    lv_slider_set_range(_slider_br, 10, 255);
    lv_slider_set_value(_slider_br, PDA.Display.getBrightness(), LV_ANIM_OFF);
    lv_obj_add_event_cb(_slider_br, _cb_slider, LV_EVENT_VALUE_CHANGED, this);

    _lbl_br_val = lv_label_create(_panel_settings);
    lv_obj_set_style_text_color(_lbl_br_val, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(_lbl_br_val, &lv_font_montserrat_14, 0);
    lv_obj_set_style_bg_opa(_lbl_br_val, LV_OPA_COVER, 0);   // TEAR-1
    lv_obj_set_style_bg_color(_lbl_br_val, lv_color_hex(0x111827), 0);
    lv_obj_set_pos(_lbl_br_val, PDA2_SCREEN_W - 70, 52);

    char buf[8];
    snprintf(buf, sizeof(buf), "%3d", PDA.Display.getBrightness());
    lv_label_set_text(_lbl_br_val, buf);

    // ── WiFi ────────────────────────────────────────────────
    _lbl_wifi = lv_label_create(_panel_settings);
    lv_label_set_text(_lbl_wifi, "WiFi");
    lv_obj_set_style_text_color(_lbl_wifi, lv_color_hex(0x9ca3af), 0);
    lv_obj_set_style_text_font(_lbl_wifi, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(_lbl_wifi, 0, 110);

    _sw_wifi = lv_switch_create(_panel_settings);
    lv_obj_set_pos(_sw_wifi, PDA2_SCREEN_W - 100, 106);
    lv_obj_add_event_cb(_sw_wifi, _cb_wifi, LV_EVENT_VALUE_CHANGED, this);
}

// ═══════════════════════════════════════════════════════════
//  openNotif / openSettings
// ═══════════════════════════════════════════════════════════

void QuickPanel_Class::openNotif() {
    if (_open) return;
    _refresh_notif_list();
    _open_panel(_panel_notif, QP_NOTIF_H);
}

void QuickPanel_Class::openSettings() {
    if (_open) return;

    // Синхронизировать яркость
    uint8_t br = PDA.Display.getBrightness();
    lv_slider_set_value(_slider_br, br, LV_ANIM_OFF);
    char buf[8];
    snprintf(buf, sizeof(buf), "%3d", br);
    lv_label_set_text(_lbl_br_val, buf);

    // Синхронизировать тогл WiFi
    if (PDA.Prefs.getWifi())
        lv_obj_add_state(_sw_wifi, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(_sw_wifi, LV_STATE_CHECKED);

    _open_panel(_panel_settings, QP_SETTINGS_H);
}

// ═══════════════════════════════════════════════════════════
//  _open_panel
// ═══════════════════════════════════════════════════════════

void QuickPanel_Class::_open_panel(lv_obj_t* panel, int32_t h) {
    _active_panel          = panel;
    _active_h              = h;
    _open                  = true;
    _ignore_overlay_click  = true;   // свайп не закрывает панель сразу

    lv_obj_clear_flag(_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_HIDDEN);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, panel);
    lv_anim_set_exec_cb(&a, [](void* obj, int32_t val) {
        lv_obj_set_y((lv_obj_t*)obj, val);
    });
    lv_anim_set_values(&a, -h, 0);
    lv_anim_set_duration(&a, QP_ANIM_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

// ═══════════════════════════════════════════════════════════
//  close
// ═══════════════════════════════════════════════════════════

void QuickPanel_Class::close() {
    if (!_open || !_active_panel) return;
    _open = false;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, _active_panel);
    lv_anim_set_exec_cb(&a, [](void* obj, int32_t val) {
        lv_obj_set_y((lv_obj_t*)obj, val);
    });
    lv_anim_set_values(&a, 0, -_active_h);
    lv_anim_set_duration(&a, QP_ANIM_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_completed_cb(&a, _cb_anim_close_done);
    lv_anim_start(&a);
}

// ═══════════════════════════════════════════════════════════
//  _onCloseDone
// ═══════════════════════════════════════════════════════════

void QuickPanel_Class::_onCloseDone() {
    if (_active_panel) {
        lv_obj_add_flag(_active_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_y(_active_panel, -_active_h);
        _active_panel = nullptr;
    }
    lv_obj_add_flag(_overlay, LV_OBJ_FLAG_HIDDEN);
}

// ═══════════════════════════════════════════════════════════
//  pushNotif / clearNotifs
// ═══════════════════════════════════════════════════════════

void QuickPanel_Class::pushNotif(const char* app_name, const char* text) {
    Notif& n = _notifs[_notif_head];
    strncpy(n.app_name, app_name, sizeof(n.app_name) - 1);
    n.app_name[sizeof(n.app_name) - 1] = '\0';
    strncpy(n.text, text, sizeof(n.text) - 1);
    n.text[sizeof(n.text) - 1] = '\0';

    pda2_time_t t = PDA.Rtc.get();
    n.hour   = t.hour;
    n.minute = t.minute;
    n.used   = true;

    _notif_head = (_notif_head + 1) % PDA2_NOTIF_MAX;
    if (_notif_count < PDA2_NOTIF_MAX) _notif_count++;

    PDA_LOGI("qpanel", "notif: [%s] %s", app_name, text);
}

void QuickPanel_Class::clearNotifs() {
    for (auto& n : _notifs) n.used = false;
    _notif_count = 0;
    _notif_head  = 0;
    if (_open && _active_panel == _panel_notif) {
        _refresh_notif_list();
    }
}

// ═══════════════════════════════════════════════════════════
//  _refresh_notif_list
// ═══════════════════════════════════════════════════════════

void QuickPanel_Class::_refresh_notif_list() {
    lv_obj_clean(_cont_list);

    int used = 0;
    for (int i = 0; i < PDA2_NOTIF_MAX; i++) if (_notifs[i].used) used++;
    if (used == 0) {
        lv_obj_clear_flag(_lbl_empty, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_add_flag(_lbl_empty, LV_OBJ_FLAG_HIDDEN);

    const int32_t card_w = PDA2_SCREEN_W - PANEL_PAD * 2;
    int32_t y = 0;

    for (int i = PDA2_NOTIF_MAX - 1; i >= 0; i--) {
        int idx = ((int)_notif_head - 1 - i + PDA2_NOTIF_MAX) % PDA2_NOTIF_MAX;
        Notif& n = _notifs[idx];
        if (!n.used) continue;

        lv_obj_t* card = lv_obj_create(_cont_list);
        lv_obj_set_size(card, card_w, CARD_H);
        lv_obj_set_pos(card, 0, y);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x1f2937), 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(card, 0, 0);
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_set_style_pad_all(card, 10, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_user_data(card, (void*)(uintptr_t)idx);
        lv_obj_add_event_cb(card, _cb_notif_click, LV_EVENT_CLICKED, this);

        lv_obj_t* lbl_app = lv_label_create(card);
        lv_label_set_text(lbl_app, n.app_name);
        lv_obj_set_style_text_color(lbl_app, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_text_font(lbl_app, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(lbl_app, 0, 0);

        lv_obj_t* lbl_time = lv_label_create(card);
        char tbuf[8];
        snprintf(tbuf, sizeof(tbuf), "%02d:%02d", n.hour, n.minute);
        lv_label_set_text(lbl_time, tbuf);
        lv_obj_set_style_text_color(lbl_time, lv_color_hex(0x6b7280), 0);
        lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_12, 0);
        lv_obj_set_style_bg_opa(lbl_time, LV_OPA_COVER, 0);   // TEAR-1
        lv_obj_set_style_bg_color(lbl_time, lv_color_hex(0x1f2937), 0);
        lv_obj_align(lbl_time, LV_ALIGN_TOP_RIGHT, 0, 0);

        lv_obj_t* lbl_text = lv_label_create(card);
        lv_label_set_text(lbl_text, n.text);
        lv_label_set_long_mode(lbl_text, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl_text, card_w - 20);
        lv_obj_set_style_text_color(lbl_text, lv_color_hex(0x9ca3af), 0);
        lv_obj_set_style_text_font(lbl_text, &lv_font_montserrat_12, 0);
        lv_obj_set_pos(lbl_text, 0, 24);

        y += CARD_H + CARD_GAP;
    }

    lv_obj_set_height(_cont_list, y > 0 ? y : 10);
}

// ═══════════════════════════════════════════════════════════
//  Callbacks
// ═══════════════════════════════════════════════════════════

void QuickPanel_Class::_cb_overlay_click(lv_event_t* e) {
    QuickPanel_Class* qp = (QuickPanel_Class*)lv_event_get_user_data(e);
    if (qp->_ignore_overlay_click) {
        qp->_ignore_overlay_click = false;
        return;
    }
    qp->close();
}

void QuickPanel_Class::_cb_slider(lv_event_t* e) {
    QuickPanel_Class* qp = (QuickPanel_Class*)lv_event_get_user_data(e);
    lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
    uint8_t val = (uint8_t)lv_slider_get_value(slider);
    PDA.Display.setBrightness(val);
    PDA.Prefs.setBrightness(val);
    char buf[8];
    snprintf(buf, sizeof(buf), "%3d", val);
    lv_label_set_text(qp->_lbl_br_val, buf);
}

void QuickPanel_Class::_cb_wifi(lv_event_t* e) {
    lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
    bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
    PDA.Prefs.setWifi(enabled);
    if (enabled) {
        WiFi.mode(WIFI_STA);
    } else {
        WiFi.mode(WIFI_OFF);
    }
    PDA_LOGI("qpanel", "WiFi %s", enabled ? "on" : "off");
}

void QuickPanel_Class::_cb_notif_click(lv_event_t* e) {
    QuickPanel_Class* qp = (QuickPanel_Class*)lv_event_get_user_data(e);
    lv_obj_t* card = (lv_obj_t*)lv_event_get_current_target(e);
    uintptr_t idx = (uintptr_t)lv_obj_get_user_data(card);
    qp->_notifs[idx].used = false;
    if (qp->_notif_count > 0) qp->_notif_count--;
    qp->_refresh_notif_list();
}

void QuickPanel_Class::_cb_anim_close_done(lv_anim_t* a) {
    lv_obj_t* panel = (lv_obj_t*)a->var;
    QuickPanel_Class* qp = (QuickPanel_Class*)lv_obj_get_user_data(panel);
    qp->_onCloseDone();
}