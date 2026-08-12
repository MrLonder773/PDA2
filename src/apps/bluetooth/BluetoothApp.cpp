// ════════════════════════════════════════════════════════
//  PDA 2 — BluetoothApp.cpp
// ════════════════════════════════════════════════════════
#include "BluetoothApp.h"

static constexpr int PAD     = 12;
static constexpr int STS_H   = 18;
static constexpr int SCN_H   = 18;
static constexpr int BTN_H   = 40;
static constexpr int ROW_H   = 36;
static constexpr int ROW_GAP = 4;

static constexpr uint32_t BG     = 0x101820;
static constexpr uint32_t BTN_BG = 0x1e3050;
static constexpr uint32_t DIS_BG = 0x301818;
static constexpr uint32_t ROW_BG = 0x182030;

// ── onInit ───────────────────────────────────────────────
void BluetoothApp::onInit() {
    screen = lv_obj_create(nullptr);
    lv_obj_set_size(screen, PDA2_SCREEN_W, PDA2_SCREEN_H);
    lv_obj_set_style_bg_color(screen, lv_color_hex(BG), 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);

    const int x = PAD;
    const int w = PDA2_SCREEN_W - PAD * 2;
    int y = PAD;

    // ── Статус ──────────────────────────────────────────
    _status_lbl = lv_label_create(screen);
    lv_obj_set_size(_status_lbl, w, STS_H);
    lv_obj_set_pos(_status_lbl, x, y);
    lv_obj_set_style_text_font(_status_lbl, PDA.Fonts.get(14), 0);
    lv_obj_set_style_text_color(_status_lbl, lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_bg_color(_status_lbl, lv_color_hex(BG), 0);
    lv_obj_set_style_bg_opa(_status_lbl, LV_OPA_COVER, 0);   // Правило 10
    lv_label_set_text(_status_lbl, "Статус: откл");
    y += STS_H + 6;

    // ── Скан / подключение ──────────────────────────────
    _scan_lbl = lv_label_create(screen);
    lv_obj_set_size(_scan_lbl, w, SCN_H);
    lv_obj_set_pos(_scan_lbl, x, y);
    lv_obj_set_style_text_font(_scan_lbl, PDA.Fonts.get(14), 0);
    lv_obj_set_style_text_color(_scan_lbl, lv_color_hex(0x88aacc), 0);
    lv_obj_set_style_bg_color(_scan_lbl, lv_color_hex(BG), 0);
    lv_obj_set_style_bg_opa(_scan_lbl, LV_OPA_COVER, 0);     // Правило 10
    lv_label_set_text(_scan_lbl, "");
    y += SCN_H + 12;

    // ── Action button ────────────────────────────────────
    _action_btn = lv_obj_create(screen);
    lv_obj_set_size(_action_btn, w, BTN_H);
    lv_obj_set_pos(_action_btn, x, y);
    lv_obj_set_style_bg_color(_action_btn, lv_color_hex(BTN_BG), 0);
    lv_obj_set_style_radius(_action_btn, 6, 0);
    lv_obj_set_style_border_width(_action_btn, 1, 0);
    lv_obj_set_style_border_color(_action_btn, lv_color_hex(0x445566), 0);
    lv_obj_set_style_pad_all(_action_btn, 0, 0);
    
    lv_obj_add_event_cb(_action_btn,  _btn_cb, LV_EVENT_CLICKED, this);

    _action_lbl = lv_label_create(_action_btn);
    lv_obj_set_style_text_font(_action_lbl, PDA.Fonts.get(16), 0);
    lv_obj_set_style_text_color(_action_lbl, lv_color_hex(0xe0e0e0), 0);
    lv_obj_set_style_bg_color(_action_lbl, lv_color_hex(BTN_BG), 0);
    lv_obj_set_style_bg_opa(_action_lbl, LV_OPA_COVER, 0);   // Правило 10
    lv_label_set_text(_action_lbl, "Вкл BT");
    lv_obj_align(_action_lbl, LV_ALIGN_LEFT_MID, 10, 0);
    y += BTN_H + ROW_GAP;

    // ── Disable button (hidden by default) ───────────────
    _disable_btn = lv_obj_create(screen);
    lv_obj_set_size(_disable_btn, w, BTN_H);
    lv_obj_set_pos(_disable_btn, x, y);
    lv_obj_set_style_bg_color(_disable_btn, lv_color_hex(DIS_BG), 0);
    lv_obj_set_style_radius(_disable_btn, 6, 0);
    lv_obj_set_style_border_width(_disable_btn, 1, 0);
    lv_obj_set_style_border_color(_disable_btn, lv_color_hex(0x664444), 0);
    lv_obj_set_style_pad_all(_disable_btn, 0, 0);
    lv_obj_add_flag(_disable_btn, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_event_cb(_disable_btn, _btn_cb, LV_EVENT_CLICKED, this);

    _disable_lbl = lv_label_create(_disable_btn);
    lv_obj_set_style_text_font(_disable_lbl, PDA.Fonts.get(16), 0);
    lv_obj_set_style_text_color(_disable_lbl, lv_color_hex(0xff8888), 0);
    lv_obj_set_style_bg_color(_disable_lbl, lv_color_hex(DIS_BG), 0);
    lv_obj_set_style_bg_opa(_disable_lbl, LV_OPA_COVER, 0);  // Правило 10
    lv_label_set_text(_disable_lbl, "Выкл BT");
    lv_obj_align(_disable_lbl, LV_ALIGN_LEFT_MID, 10, 0);
    y += BTN_H + 8;

    // ── Device rows ──────────────────────────────────────
    for (uint8_t i = 0; i < ROWS_MAX; i++) {
        _dev_rows[i] = lv_obj_create(screen);
        lv_obj_set_size(_dev_rows[i], w, ROW_H);
        lv_obj_set_pos(_dev_rows[i], x, y + i * (ROW_H + ROW_GAP));
        lv_obj_set_style_bg_color(_dev_rows[i], lv_color_hex(ROW_BG), 0);
        lv_obj_set_style_radius(_dev_rows[i], 4, 0);
        lv_obj_set_style_border_width(_dev_rows[i], 1, 0);
        lv_obj_set_style_border_color(_dev_rows[i], lv_color_hex(0x334455), 0);
        lv_obj_set_style_pad_all(_dev_rows[i], 0, 0);

        lv_obj_add_event_cb(_dev_rows[i], _btn_cb, LV_EVENT_CLICKED, this);

        _dev_lbls[i] = lv_label_create(_dev_rows[i]);
        lv_obj_set_style_text_font(_dev_lbls[i], PDA.Fonts.get(14), 0);
        lv_obj_set_style_text_color(_dev_lbls[i], lv_color_hex(0xcccccc), 0);
        lv_obj_set_style_bg_color(_dev_lbls[i], lv_color_hex(ROW_BG), 0);
        lv_obj_set_style_bg_opa(_dev_lbls[i], LV_OPA_COVER, 0);  // Правило 10
        lv_label_set_long_mode(_dev_lbls[i], LV_LABEL_LONG_CLIP);
        lv_obj_set_width(_dev_lbls[i], w - 12);
        lv_obj_align(_dev_lbls[i], LV_ALIGN_LEFT_MID, 8, 0);
        lv_label_set_text(_dev_lbls[i], "");
        lv_obj_add_flag(_dev_rows[i], LV_OBJ_FLAG_HIDDEN);
    }
}

// ── onOpen ───────────────────────────────────────────────
void BluetoothApp::onOpen() {
    _focus       = 0;
    _last_state  = (BtState)0xFF;   // форсируем rebuild
    _last_devcnt = 0xFF;
}

// ── onClose ──────────────────────────────────────────────
void BluetoothApp::onClose() {
    // BT продолжает работать в фоне
}

// ── onTick ───────────────────────────────────────────────
void BluetoothApp::onTick(uint32_t) {
    BtState st  = PDA.Bt.state();
    uint8_t cnt = PDA.Bt.deviceCount();
    if (st != _last_state || cnt != _last_devcnt) {
        _last_state  = st;
        _last_devcnt = cnt;
        _rebuildUi();
    }
}

// ── onKey ────────────────────────────────────────────────
void BluetoothApp::onKey(uint8_t keycode, uint8_t modifier) {
    switch (keycode) {
        case 82: case 80:   // ↑ ←
            _focus = (_focus == 0) ? _focus_max : _focus - 1;
            _updateFocus();
            break;
        case 81: case 79:   // ↓ →
            _focus = (_focus >= _focus_max) ? 0 : _focus + 1;
            _updateFocus();
            break;
        case 40:            // Enter
            _doAction();
            break;
    }
}

// ── _doAction ────────────────────────────────────────────
void BluetoothApp::_doAction() {
    if (_focus == 0) {
        switch (PDA.Bt.state()) {
            case BT_OFF:
                PDA.Bt.enable();
                PDA.Prefs.setBtEnabled(true);
                break;
            case BT_IDLE:
            case BT_CONNECTED:
                PDA.Bt.startScan();
                break;
            case BT_SCANNING:
                PDA.Bt.stopScan();
                break;
        }
    } else if (_focus == 1) {
        // [Выкл BT] — видна только когда BT != OFF
        PDA.Bt.disable();
        PDA.Prefs.setBtEnabled(false);
    } else {
        uint8_t idx = _focus - 2;
        if (idx >= PDA.Bt.deviceCount()) return;
        const BtDevice& d = PDA.Bt.device(idx);
        if (d.connected) {
            PDA.Bt.disconnect();
            PDA.Prefs.setBtLastDeviceName("");
        } else {
            if (PDA.Bt.connect(idx))
                PDA.Prefs.setBtLastDeviceName(d.name);
        }
    }
}

// ── _rebuildUi ───────────────────────────────────────────
void BluetoothApp::_rebuildUi() {
    BtState st  = PDA.Bt.state();
    uint8_t cnt = PDA.Bt.deviceCount();
    bool bt_on  = (st != BT_OFF);

    _updateStatus();

    // Action button текст
    const char* btn_txt = "Вкл BT";
    if      (st == BT_SCANNING) btn_txt = "Стоп";
    else if (bt_on)             btn_txt = "Сканировать";
    lv_label_set_text(_action_lbl, btn_txt);

    // Disable button — только когда BT включён
    if (bt_on) lv_obj_clear_flag(_disable_btn, LV_OBJ_FLAG_HIDDEN);
    else       lv_obj_add_flag  (_disable_btn, LV_OBJ_FLAG_HIDDEN);

    // Device rows
    // ⚠️ Если ✓ не рендерится — заменить "\xE2\x9C\x93" на "(ok)"
    for (uint8_t i = 0; i < ROWS_MAX; i++) {
        if (i < cnt) {
            const BtDevice& d = PDA.Bt.device(i);
            char buf[36];
            if (d.connected)
                snprintf(buf, sizeof(buf), "%s (ok)", d.name);
            else
                snprintf(buf, sizeof(buf), "%s", d.name);
            lv_label_set_text(_dev_lbls[i], buf);
            lv_obj_clear_flag(_dev_rows[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(_dev_rows[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    // focus_max: OFF → 0; иначе → 1(disable) + cnt(девайсы)
    _focus_max = bt_on ? (1 + cnt) : 0;
    if (_focus > _focus_max) _focus = 0;
    _updateFocus();
}

// ── _updateStatus ────────────────────────────────────────
void BluetoothApp::_updateStatus() {
    switch (PDA.Bt.state()) {
        case BT_OFF:
            lv_label_set_text(_status_lbl, "Статус: откл");
            lv_label_set_text(_scan_lbl,   "");
            break;
        case BT_IDLE:
            lv_label_set_text(_status_lbl, "Статус: вкл");
            lv_label_set_text(_scan_lbl,   "");
            break;
        case BT_SCANNING:
            lv_label_set_text(_status_lbl, "Статус: вкл");
            lv_label_set_text(_scan_lbl,   "Сканирование...");
            break;
        case BT_CONNECTED: {
            lv_label_set_text(_status_lbl, "Статус: вкл");
            char buf[48];
            snprintf(buf, sizeof(buf), "Подкл: %s", PDA.Bt.connectedName());
            lv_label_set_text(_scan_lbl, buf);
            break;
        }
    }
}

// ── _updateFocus ─────────────────────────────────────────
void BluetoothApp::_updateFocus() {
    const lv_color_t focus_c = lv_color_hex(0x4488ff);

    bool f0 = (_focus == 0);
    lv_obj_set_style_border_color(_action_btn, f0 ? focus_c : lv_color_hex(0x445566), 0);
    lv_obj_set_style_border_width(_action_btn, f0 ? 2 : 1, 0);

    bool f1 = (_focus == 1);
    lv_obj_set_style_border_color(_disable_btn, f1 ? focus_c : lv_color_hex(0x664444), 0);
    lv_obj_set_style_border_width(_disable_btn, f1 ? 2 : 1, 0);

    for (uint8_t i = 0; i < ROWS_MAX; i++) {
        bool f = (_focus == i + 2);
        lv_obj_set_style_border_color(_dev_rows[i], f ? focus_c : lv_color_hex(0x334455), 0);
        lv_obj_set_style_border_width(_dev_rows[i], f ? 2 : 1, 0);
    }
}

void BluetoothApp::_btn_cb(lv_event_t* e) {
    auto* app = static_cast<BluetoothApp*>(lv_event_get_user_data(e));
    lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));

    if      (target == app->_action_btn)  app->_focus = 0;
    else if (target == app->_disable_btn) app->_focus = 1;
    else {
        for (uint8_t i = 0; i < ROWS_MAX; i++)
            if (target == app->_dev_rows[i]) { app->_focus = i + 2; break; }
    }
    app->_updateFocus();
    app->_doAction();
}