// src/apps/chat/ChatApp.cpp — PDA 2 / PDA 2 Lite
/*
#include "ChatApp.h"
#include "pda2_log.h"
#include <string.h>
#include <stdio.h>
#include <esp_heap_caps.h>

#define TAG "chat"

// ── Цвета (тёмная тема) ───────────────────────────────────
#define COL_BG      lv_color_hex(0x1C1C1E)
#define COL_HDR     lv_color_hex(0x2C2C2E)
#define COL_IN_BUB  lv_color_hex(0x3A3A3C)
#define COL_OUT_BUB lv_color_hex(0x1D5FA6)
#define COL_FOCUS   lv_color_hex(0xFFD60A)
#define COL_BTN     lv_color_hex(0x444446)

ChatApp* ChatApp::_s_this = nullptr;

// ══════════════════════════════════════════════════════════
//  Constructor
// ══════════════════════════════════════════════════════════

ChatApp::ChatApp(uint8_t own_id, uint8_t peer_id)
    : _own_id(own_id), _peer_id(peer_id) {}

// ══════════════════════════════════════════════════════════
//  Touch callbacks (main PDA2 only)
// ══════════════════════════════════════════════════════════

#ifndef PDA2_LITE

void ChatApp::_cb_back(lv_event_t*) {
    PDA.Apps.close();
}

void ChatApp::_cb_lang(lv_event_t*) {
    if (!_s_this) return;
    _s_this->_lang_ru = !_s_this->_lang_ru;
    if (_s_this->_lbl_lang)
        lv_label_set_text(_s_this->_lbl_lang,
                          _s_this->_lang_ru ? "RU" : "EN");
    _s_this->_update_input_label();
}
#ifndef PDA2_LITE
void ChatApp::_cb_ptt(lv_event_t*) {
    if (!_s_this) return;
    _s_this->onButton(1);   // та же логика что и физическая кнопка
}
#endif
void ChatApp::_cb_send(lv_event_t*) {
    if (!_s_this) return;
    ChatApp* t = _s_this;
    if (t->_text_len > 0 && !t->_tx.active) {
        t->_tx_begin_text();
        t->_send_status(ChatProtocol::STATUS_IDLE);
        t->_update_input_label();
    }
}

void ChatApp::_cb_bubble(lv_event_t* e) {
    if (!_s_this) return;
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    if (idx >= _s_this->_msg_count) return;
    MsgEntry& m = _s_this->_msgs[idx];
    if (m.type == ChatProtocol::TYPE_VOICE &&
        !_s_this->_playing && !_s_this->_recording) {
        _s_this->_startPlay(m.device_id, m.msg_id);
        if (!m.outgoing) _s_this->_send_read(m.msg_id);
    }
}

#endif  // PDA2_LITE

// ══════════════════════════════════════════════════════════
//  onInit — UI + nRF24
// ══════════════════════════════════════════════════════════

void ChatApp::onInit() {
    _s_this = this;
    screen  = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen, COL_BG, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);

#ifdef PDA2_LITE
    // ── Header ────────────────────────────────────────────
    lv_obj_t* hdr = lv_obj_create(screen);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_size(hdr, CHAT_SCR_W, CHAT_HDR_H);
    lv_obj_set_style_bg_color(hdr, COL_HDR, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 0, 0);

    _lbl_header = lv_label_create(hdr);
    lv_obj_set_style_text_font(_lbl_header, PDA.Fonts.get(12), 0);
    lv_obj_set_style_text_color(_lbl_header, lv_color_white(), 0);
    lv_obj_set_style_bg_color(_lbl_header, COL_HDR, 0);
    lv_obj_set_style_bg_opa(_lbl_header, LV_OPA_COVER, 0);    // ПРАВИЛО 10
    lv_obj_set_width(_lbl_header, CHAT_SCR_W - 8);
    lv_label_set_long_mode(_lbl_header, LV_LABEL_LONG_CLIP);
    lv_label_set_text(_lbl_header, "Chat");
    lv_obj_align(_lbl_header, LV_ALIGN_LEFT_MID, 4, 0);

    // ── Список ────────────────────────────────────────────
    _list_cont = lv_obj_create(screen);
    lv_obj_set_pos(_list_cont, 0, CHAT_HDR_H);
    lv_obj_set_size(_list_cont, CHAT_SCR_W, CHAT_LIST_H);
    lv_obj_set_style_bg_color(_list_cont, COL_BG, 0);
    lv_obj_set_style_bg_opa(_list_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_list_cont, 0, 0);
    lv_obj_set_style_pad_all(_list_cont, 2, 0);
    lv_obj_set_style_pad_row(_list_cont, 3, 0);
    lv_obj_set_style_radius(_list_cont, 0, 0);
    lv_obj_set_scroll_dir(_list_cont, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(_list_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(_list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_list_cont, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // ── Input ─────────────────────────────────────────────
    lv_obj_t* inp = lv_obj_create(screen);
    lv_obj_set_pos(inp, 0, CHAT_HDR_H + CHAT_LIST_H);
    lv_obj_set_size(inp, CHAT_SCR_W, CHAT_INP_H);
    lv_obj_set_style_bg_color(inp, COL_HDR, 0);
    lv_obj_set_style_bg_opa(inp, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(inp, 0, 0);
    lv_obj_set_style_pad_all(inp, 0, 0);
    lv_obj_set_style_radius(inp, 0, 0);

    _lbl_input = lv_label_create(inp);
    lv_obj_set_style_text_font(_lbl_input, PDA.Fonts.get(12), 0);
    lv_obj_set_style_text_color(_lbl_input, lv_color_white(), 0);
    lv_obj_set_style_bg_color(_lbl_input, COL_HDR, 0);
    lv_obj_set_style_bg_opa(_lbl_input, LV_OPA_COVER, 0);     // ПРАВИЛО 10
    lv_obj_set_width(_lbl_input, CHAT_SCR_W - 8);
    lv_label_set_long_mode(_lbl_input, LV_LABEL_LONG_CLIP);
    lv_label_set_text(_lbl_input, "_");
    lv_obj_align(_lbl_input, LV_ALIGN_LEFT_MID, 4, 0);

    // ── Buttons (визуальные) ──────────────────────────────
    lv_obj_t* btn_row = lv_obj_create(screen);
    lv_obj_set_pos(btn_row, 0, CHAT_HDR_H + CHAT_LIST_H + CHAT_INP_H);
    lv_obj_set_size(btn_row, CHAT_SCR_W, CHAT_BTN_H);
    lv_obj_set_style_bg_color(btn_row, COL_BG, 0);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_set_style_radius(btn_row, 0, 0);

    auto make_btn_lite = [&](lv_coord_t x, lv_coord_t w, const char* label) {
        lv_obj_t* b = lv_obj_create(btn_row);
        lv_obj_set_pos(b, x, 2);
        lv_obj_set_size(b, w, CHAT_BTN_H - 4);
        lv_obj_set_style_bg_color(b, COL_BTN, 0);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(b, 0, 0);
        lv_obj_set_style_radius(b, 4, 0);
        lv_obj_t* lbl = lv_label_create(b);
        lv_obj_set_style_text_font(lbl, PDA.Fonts.get(12), 0);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_label_set_text(lbl, label);
        lv_obj_center(lbl);
    };
    make_btn_lite(2,  58, "PTT");
    make_btn_lite(68, 58, "ОТПР");

#else   // ══════════════════ main pda2 (320×480) ══════════════════

    // ── Header (44px): ← | Chat/статус | [EN/RU] ─────────
    lv_obj_t* hdr = lv_obj_create(screen);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_size(hdr, CHAT_SCR_W, CHAT_HDR_H);
    lv_obj_set_style_bg_color(hdr, COL_HDR, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 0, 0);

    // Кнопка ←
    lv_obj_t* btn_back = lv_obj_create(hdr);
    lv_obj_set_pos(btn_back, 0, 0);
    lv_obj_set_size(btn_back, CHAT_HDR_H, CHAT_HDR_H);
    lv_obj_set_style_bg_color(btn_back, COL_HDR, 0);
    lv_obj_set_style_bg_opa(btn_back, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_back, 0, 0);
    lv_obj_set_style_radius(btn_back, 0, 0);
    lv_obj_add_event_cb(btn_back, _cb_back, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_back = lv_label_create(btn_back);
    lv_obj_set_style_text_font(lbl_back, PDA.Fonts.get(20), 0);
    lv_obj_set_style_text_color(lbl_back, lv_color_white(), 0);
    lv_label_set_text(lbl_back, "<");
    lv_obj_center(lbl_back);

    // Кнопка [EN/RU]
    lv_obj_t* btn_lang = lv_obj_create(hdr);
    lv_obj_set_pos(btn_lang, CHAT_SCR_W - 64, 4);
    lv_obj_set_size(btn_lang, 60, CHAT_HDR_H - 8);
    lv_obj_set_style_bg_color(btn_lang, COL_BTN, 0);
    lv_obj_set_style_bg_opa(btn_lang, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_lang, 0, 0);
    lv_obj_set_style_radius(btn_lang, 6, 0);
    lv_obj_add_event_cb(btn_lang, _cb_lang, LV_EVENT_CLICKED, nullptr);
    _lbl_lang = lv_label_create(btn_lang);
    lv_obj_set_style_text_font(_lbl_lang, PDA.Fonts.get(16), 0);
    lv_obj_set_style_text_color(_lbl_lang, lv_color_white(), 0);
    lv_obj_set_style_bg_color(_lbl_lang, COL_BTN, 0);
    lv_obj_set_style_bg_opa(_lbl_lang, LV_OPA_COVER, 0);    // ПРАВИЛО 10
    lv_label_set_text(_lbl_lang, "EN");
    lv_obj_center(_lbl_lang);

    // Заголовок / статус
    _lbl_header = lv_label_create(hdr);
    lv_obj_set_style_text_font(_lbl_header, PDA.Fonts.get(20), 0);
    lv_obj_set_style_text_color(_lbl_header, lv_color_white(), 0);
    lv_obj_set_style_bg_color(_lbl_header, COL_HDR, 0);
    lv_obj_set_style_bg_opa(_lbl_header, LV_OPA_COVER, 0);  // ПРАВИЛО 10
    lv_obj_set_pos(_lbl_header, CHAT_HDR_H + 4, 0);
    lv_obj_set_size(_lbl_header, CHAT_SCR_W - CHAT_HDR_H - 68, CHAT_HDR_H);
    lv_label_set_long_mode(_lbl_header, LV_LABEL_LONG_CLIP);
    lv_label_set_text(_lbl_header, "Chat");
    lv_obj_set_style_text_align(_lbl_header, LV_TEXT_ALIGN_LEFT, 0);
    // Вертикальное выравнивание по центру через pad
    lv_obj_set_style_pad_top(_lbl_header,
        (CHAT_HDR_H - lv_font_get_line_height(PDA.Fonts.get(20))) / 2, 0);

    // ── Список сообщений ──────────────────────────────────
    _list_cont = lv_obj_create(screen);
    lv_obj_set_pos(_list_cont, 0, CHAT_HDR_H);
    lv_obj_set_size(_list_cont, CHAT_SCR_W, CHAT_LIST_H);
    lv_obj_set_style_bg_color(_list_cont, COL_BG, 0);
    lv_obj_set_style_bg_opa(_list_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_list_cont, 0, 0);
    lv_obj_set_style_pad_all(_list_cont, 4, 0);
    lv_obj_set_style_pad_row(_list_cont, 6, 0);
    lv_obj_set_style_radius(_list_cont, 0, 0);
    lv_obj_set_scroll_dir(_list_cont, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(_list_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(_list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_list_cont, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // ── Input (56px) ──────────────────────────────────────
    lv_obj_t* inp = lv_obj_create(screen);
    lv_obj_set_pos(inp, 0, CHAT_HDR_H + CHAT_LIST_H);
    lv_obj_set_size(inp, CHAT_SCR_W, CHAT_INP_H);
    lv_obj_set_style_bg_color(inp, COL_HDR, 0);
    lv_obj_set_style_bg_opa(inp, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(inp, 0, 0);
    lv_obj_set_style_pad_left(inp, 8, 0);
    lv_obj_set_style_pad_top(inp, 0, 0);
    lv_obj_set_style_radius(inp, 0, 0);

    _lbl_input = lv_label_create(inp);
    lv_obj_set_style_text_font(_lbl_input, PDA.Fonts.get(16), 0);
    lv_obj_set_style_text_color(_lbl_input, lv_color_white(), 0);
    lv_obj_set_style_bg_color(_lbl_input, COL_HDR, 0);
    lv_obj_set_style_bg_opa(_lbl_input, LV_OPA_COVER, 0);   // ПРАВИЛО 10
    lv_obj_set_width(_lbl_input, CHAT_SCR_W - 16);
    lv_label_set_long_mode(_lbl_input, LV_LABEL_LONG_CLIP);
    lv_label_set_text(_lbl_input, "_");
    lv_obj_align(_lbl_input, LV_ALIGN_LEFT_MID, 0, 0);

    // ── Кнопки (64px, тач) ───────────────────────────────
    lv_obj_t* btn_row = lv_obj_create(screen);
    lv_obj_set_pos(btn_row, 0, CHAT_HDR_H + CHAT_LIST_H + CHAT_INP_H);
    lv_obj_set_size(btn_row, CHAT_SCR_W, CHAT_BTN_H);
    lv_obj_set_style_bg_color(btn_row, COL_BG, 0);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_set_style_radius(btn_row, 0, 0);

    lv_coord_t bw = CHAT_SCR_W / 2 - 6;
    lv_coord_t bh = CHAT_BTN_H - 8;

    _btn_ptt = lv_obj_create(btn_row);
    lv_obj_set_pos(_btn_ptt, 4, 4);
    lv_obj_set_size(_btn_ptt, bw, bh);
    lv_obj_set_style_bg_color(_btn_ptt, COL_BTN, 0);
    lv_obj_set_style_bg_opa(_btn_ptt, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_btn_ptt, 0, 0);
    lv_obj_set_style_radius(_btn_ptt, 8, 0);
    lv_obj_add_event_cb(_btn_ptt, _cb_ptt, LV_EVENT_CLICKED, nullptr);
    _lbl_ptt = lv_label_create(_btn_ptt);
    lv_obj_set_style_text_font(_lbl_ptt, PDA.Fonts.get(16), 0);
    lv_obj_set_style_text_color(_lbl_ptt, lv_color_white(), 0);
    lv_obj_set_style_bg_color(_lbl_ptt, COL_BTN, 0);
    lv_obj_set_style_bg_opa(_lbl_ptt, LV_OPA_COVER, 0);     // ПРАВИЛО 10
    lv_label_set_text(_lbl_ptt, "PTT");
    lv_obj_center(_lbl_ptt);

    _btn_send = lv_obj_create(btn_row);
    lv_obj_set_pos(_btn_send, CHAT_SCR_W / 2 + 2, 4);
    lv_obj_set_size(_btn_send, bw, bh);
    lv_obj_set_style_bg_color(_btn_send, COL_OUT_BUB, 0);
    lv_obj_set_style_bg_opa(_btn_send, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_btn_send, 0, 0);
    lv_obj_set_style_radius(_btn_send, 8, 0);
    lv_obj_add_event_cb(_btn_send, _cb_send, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_send = lv_label_create(_btn_send);
    lv_obj_set_style_text_font(lbl_send, PDA.Fonts.get(16), 0);
    lv_obj_set_style_text_color(lbl_send, lv_color_white(), 0);
    lv_label_set_text(lbl_send, "ОТПР");
    lv_obj_center(lbl_send);

#endif  // PDA2_LITE

    // ── nRF24 init (общее) ────────────────────────────────
    NrfConfig cfg;
    cfg.channel      = ChatProtocol::CHAT_CHANNEL;
    cfg.payload_size = ChatProtocol::PKT_SIZE;
    cfg.pa_level     = ChatProtocol::CHAT_PA_LEVEL;
    cfg.data_rate    = ChatProtocol::CHAT_DATA_RATE;
    memcpy(cfg.addr, ChatProtocol::CHAT_ADDR, 5);

    // if (!PDA.Nrf.begin(PDA.Wroom, cfg)) {
    //     PDA_LOGE(TAG, "Nrf.begin failed");
    //     lv_label_set_text(_lbl_header, "nRF24 ERR");
    //     return;
    // }
    // PDA.Nrf.onPacket(_on_packet);

    // _ensure_dir(_own_id);
    // _ensure_dir(_peer_id);
    // PDA_LOGI(TAG, "ready own=0x%02X peer=0x%02X", _own_id, _peer_id);
}

// ══════════════════════════════════════════════════════════
//  onOpen / onClose
// ══════════════════════════════════════════════════════════

void ChatApp::onOpen() {
    if (!_nrf_ready) {
        NrfConfig cfg;
        cfg.channel      = ChatProtocol::CHAT_CHANNEL;
        cfg.payload_size = ChatProtocol::PKT_SIZE;
        cfg.pa_level     = ChatProtocol::CHAT_PA_LEVEL;
        cfg.data_rate    = ChatProtocol::CHAT_DATA_RATE;
        memcpy(cfg.addr, ChatProtocol::CHAT_ADDR, 5);

        if (!PDA.Nrf.begin(PDA.Wroom, cfg)) {  // ← единственное изменение
            lv_label_set_text(_lbl_header, "nRF24 ERR");
            return;
        }
        PDA.Nrf.onPacket(_on_packet);
        _ensure_dir(_own_id);
        _ensure_dir(_peer_id);
        PDA_LOGI(TAG, "ready own=0x%02X peer=0x%02X", _own_id, _peer_id);
        _nrf_ready = true;
    }

    _tx_q = xQueueCreate(8, sizeof(Walkie::TxFrame));
    _rx_q = xQueueCreate(8, sizeof(Walkie::RxFrame));
    if (!_audio.begin(_tx_q, _rx_q)) {
        lv_label_set_text(_lbl_header, "Audio ERR");
        return;
    }
    _update_header();
}

void ChatApp::onClose() {
    _send_status(ChatProtocol::STATUS_IDLE);
    _s_this = nullptr;
    if (_recording) { _recording = false; _audio.stopCapture(); }
    if (_playing)   _stopPlay();
    if (_tx.active) {
        if (_rec_buf) { heap_caps_free(_rec_buf); _rec_buf = nullptr; }
        _tx = TxState{};
    }
    _rx_reset();
    _audio.end();
    if (_tx_q) { vQueueDelete(_tx_q); _tx_q = nullptr; }
    if (_rx_q) { vQueueDelete(_rx_q); _rx_q = nullptr; }
    _text_len    = 0;
    _text_buf[0] = 0;
    _msg_count   = 0;
    _focus_idx   = -1;
    _input_active = true;
}

// ══════════════════════════════════════════════════════════
//  onTick
// ══════════════════════════════════════════════════════════

void ChatApp::onTick(uint32_t delta_ms) {
    // ── Курсор мигает ─────────────────────────────────────
    _cursor_blink_ms += delta_ms;
    if (_cursor_blink_ms >= 500) {
        _cursor_blink_ms = 0;
        _cursor_visible  = !_cursor_visible;
        _update_input_label();
    }

    // ── Таймаут статуса peer-а ────────────────────────────
    if (_peer_status != ChatProtocol::STATUS_IDLE) {
        _peer_status_age += delta_ms;
        if (_peer_status_age > 4000) {
            _peer_status = ChatProtocol::STATUS_IDLE;
            _update_header();
        }
    }

    // ── Периодически шлём STATUS_TYPING ──────────────────
    if (_text_len > 0 && !_tx.active && _input_active) {
        _typing_timer += delta_ms;
        if (_typing_timer >= 1000) {
            _typing_timer = 0;
            _send_status(ChatProtocol::STATUS_TYPING);
        }
    }

    // ── 1. Дренируем TxFrames от audio task → _rec_buf ──
    if (_recording && _rec_buf) {
        Walkie::TxFrame tf;
        while (xQueueReceive(_tx_q, &tf, 0) == pdTRUE) {
            if (_first_rec_frame) {
                _rec_initial_pred = tf.pred;
                _rec_initial_step = tf.step_idx;
                _first_rec_frame  = false;
            }
            size_t chunk = sizeof(tf.data);
            if (_rec_buf_pos + chunk > REC_MAX + 3) {
                PDA_LOGW(TAG, "rec: buf full");
                _stopRecord();
                break;
            }
            memcpy(_rec_buf + _rec_buf_pos, tf.data, chunk);
            _rec_buf_pos += chunk;
        }
    }

    // ── 2. Пушим RxFrames для воспроизведения ────────────
    if (_playing && _play_buf) {
        const uint8_t* adpcm_data  = _play_buf + 3;
        size_t         adpcm_total = _play_buf_size - 3;
        size_t         chunk       = sizeof(Walkie::RxFrame::data);

        while (_play_adpcm_pos < adpcm_total) {
            Walkie::RxFrame f;
            f.pred     = _play_state.pred;
            f.step_idx = (uint8_t)_play_state.index;

            size_t copy = adpcm_total - _play_adpcm_pos;
            if (copy > chunk) copy = chunk;
            memcpy(f.data, adpcm_data + _play_adpcm_pos, copy);
            if (copy < chunk) memset(f.data + copy, 0, chunk - copy);

            if (xQueueSend(_rx_q, &f, 0) != pdTRUE) break;

            int16_t dummy[Walkie::PCM_SAMPLES];
            ADPCM::decode(f.data, dummy, Walkie::PCM_SAMPLES, _play_state);
            _play_adpcm_pos += copy;
        }

        if (_audio.taskHandle())
            xTaskNotify(_audio.taskHandle(), WAL_PLAY_BIT, eSetBits);

        if (_play_adpcm_pos >= adpcm_total) {
            heap_caps_free(_play_buf);
            _play_buf = nullptr;
            _playing  = false;
            _update_header();
        }
    }

    // ── 3. TX nRF ─────────────────────────────────────────
    if (_tx.active) _tx_step();
}

// Таблица RU: позиции a-z (kc 4-29) → Unicode codepoint кириллицы (нижний регистр)
static const uint16_t _ru_cp[26] = {
    0x0444, // a → ф
    0x0438, // b → и
    0x0441, // c → с
    0x0432, // d → в
    0x0443, // e → у
    0x0430, // f → а
    0x043F, // g → п
    0x0440, // h → р
    0x0448, // i → ш
    0x043E, // j → о
    0x043B, // k → л
    0x0434, // l → д
    0x044C, // m → ь
    0x0442, // n → т
    0x0449, // o → щ
    0x0437, // p → з
    0x0439, // q → й
    0x043A, // r → к
    0x044B, // s → ы
    0x0435, // t → е
    0x0433, // u → г
    0x043C, // v → м
    0x0446, // w → ц
    0x0447, // x → ч
    0x043D, // y → н
    0x044F, // z → я
};

// Записывает 1-2 байта в buf[pos], возвращает кол-во добавленных байт
static uint8_t _keycode_append(uint8_t kc, uint8_t mod, bool ru,
                                char* buf, size_t pos, size_t max) {
    bool shift = mod & 0x22;  // LShift | RShift

    if (!ru) {
        char c = 0;
        if (kc >= 4  && kc <= 29) { c = 'a' + (kc - 4); if (shift) c -= 32; }
        else if (kc >= 30 && kc <= 38) {
            const char n[] = "123456789", s[] = "!@#$%^&*(";
            c = shift ? s[kc - 30] : n[kc - 30];
        }
        else if (kc == 39) c = shift ? ')' : '0';
        else if (kc == 44) c = ' ';
        else if (kc == 45) c = shift ? '_' : '-';
        else if (kc == 46) c = shift ? '+' : '=';
        else if (kc == 51) c = shift ? ':' : ';';
        else if (kc == 54) c = shift ? '<' : ',';
        else if (kc == 55) c = shift ? '>' : '.';
        else if (kc == 56) c = shift ? '?' : '/';
        if (c && pos + 1 < max) { buf[pos] = c; return 1; }
        return 0;
    }

    // RU — UTF-8 (2 байта)
    if (kc >= 4 && kc <= 29 && pos + 2 < max) {
        uint16_t cp = _ru_cp[kc - 4];
        if (shift) cp -= 0x20;  // нижний → верхний регистр
        buf[pos]     = (char)(0xC0 | (cp >> 6));
        buf[pos + 1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (kc == 44 && pos + 1 < max) { buf[pos] = ' '; return 1; }
    return 0;
}

// ══════════════════════════════════════════════════════════
//  onButton
// ══════════════════════════════════════════════════════════
#ifndef PDA2_LITE
void ChatApp::onButton(uint8_t clicks) {
    if (clicks == 1) {
        if (_playing) {
            _stopPlay();
        } else if (_recording) {
            _stopRecord();
        } else if (_text_len > 0 && !_tx.active) {
            // Есть текст → отправить
            _tx_begin_text();
            _send_status(ChatProtocol::STATUS_IDLE);
            _update_input_label();
        } else {
            _startRecord();
        }
    } else if (clicks == 2) {
        // Воспроизвести последнее входящее голосовое
        if (!_playing && !_recording) {
            for (int8_t i = (int8_t)_msg_count - 1; i >= 0; i--) {
                if (!_msgs[i].outgoing &&
                     _msgs[i].type == ChatProtocol::TYPE_VOICE) {
                    _startPlay(_msgs[i].device_id, _msgs[i].msg_id);
                    _send_read(_msgs[i].msg_id);
                    break;
                }
            }
        }
    } else if (clicks == 3) {
        PDA.Apps.close();
    }
}
#endif
// ══════════════════════════════════════════════════════════
//  onKey — ввод текста + навигация по пузырям
// ══════════════════════════════════════════════════════════

void ChatApp::onKey(uint8_t keycode, uint8_t modifier) {
    // ── ↑ / ↓ — навигация ────────────────────────────────
    if (keycode == 82) {  // ↑
        if (_input_active) {
            if (_msg_count > 0) _set_focus((int8_t)(_msg_count - 1));
        } else if (_focus_idx > 0) {
            _set_focus(_focus_idx - 1);
        }
        return;
    }
    if (keycode == 81) {  // ↓
        if (!_input_active) {
            if (_focus_idx < (int8_t)(_msg_count - 1))
                _set_focus(_focus_idx + 1);
            else
                _set_focus(-1);  // обратно к полю ввода
        }
        return;
    }

    // ── Enter ─────────────────────────────────────────────
    if (keycode == 40) {
        if (!_input_active && _focus_idx >= 0) {
            // Воспроизвести голосовое сообщение в фокусе
            MsgEntry& e = _msgs[_focus_idx];
            if (e.type == ChatProtocol::TYPE_VOICE && !_playing && !_recording) {
                _startPlay(e.device_id, e.msg_id);
                if (!e.outgoing) _send_read(e.msg_id);
            }
        } else if (_input_active && _text_len > 0 && !_tx.active) {
            _tx_begin_text();
            _send_status(ChatProtocol::STATUS_IDLE);
            _update_input_label();
        }
        return;
    }

    // Esc (41) перехватывается Apps_Class → close(). Сюда не доходит.
    // В режиме навигации текстовый ввод блокирован.
    if (!_input_active) return;
    if (_tx.active)     return;

    // ── Backspace ─────────────────────────────────────────
    if (keycode == 42) {
        if (_text_len > 0) {
            _text_len--;
            // UTF-8 safe: откатываемся до начала multi-byte char
            while (_text_len > 0 && (_text_buf[_text_len] & 0xC0) == 0x80)
                _text_len--;
            _text_buf[_text_len] = 0;
            if (_text_len == 0) {
                _send_status(ChatProtocol::STATUS_IDLE);
                _typing_timer = 0;
            }
            _update_input_label();
        }
        return;
    }

    // ── Ctrl+Space → переключение языка ──────────────────
    if (keycode == 44 && (modifier & 0x11)) {
        _lang_ru = !_lang_ru;
#ifndef PDA2_LITE
        if (_lbl_lang)
            lv_label_set_text(_lbl_lang, _lang_ru ? "RU" : "EN");
#endif
        _update_header();
        _update_input_label();
        return;
    }

    // ── Символ ────────────────────────────────────────────
    uint8_t added = _keycode_append(keycode, modifier, _lang_ru,
                                    _text_buf, _text_len,
                                    sizeof(_text_buf) - 1);
    if (added > 0) {
        _text_len += added;
        _text_buf[_text_len] = 0;
        _update_input_label();
    }
}

// ══════════════════════════════════════════════════════════
//  RX — колбэк + обработка пакетов
// ══════════════════════════════════════════════════════════

void ChatApp::_on_packet(const uint8_t* data, uint8_t len) {
    if (_s_this) _s_this->_process_packet(data, len);
}

void ChatApp::_process_packet(const uint8_t* data, uint8_t len) {
    using namespace ChatProtocol;
    if (len < HDR_SIZE) return;

    Header h;
    unpack_header(data, h);
    const uint8_t* pld = data + HDR_SIZE;

    if (h.device_id == _own_id) return;

    // ── TYPE_STATUS ───────────────────────────────────────
    if (h.type == TYPE_STATUS) {
        if (h.device_id != _peer_id) return;
        _peer_status     = pld[0];
        _peer_status_age = 0;
        _update_header();
        return;
    }

    // ── TYPE_READ ─────────────────────────────────────────
    if (h.type == TYPE_READ) {
        if (h.device_id != _peer_id) return;
        _mark_read(h.msg_id);
        return;
    }

    // ── TYPE_END ──────────────────────────────────────────
    if (h.type == TYPE_END) {
        if (_rx.active && _rx.device_id == h.device_id && _rx.msg_id == h.msg_id)
            _rx_flush();
        return;
    }

    // ── seq==0: новое сообщение ───────────────────────────
    if (h.seq == 0) {
        if (_rx.active) { PDA_LOGW(TAG, "rx: abort prev"); _rx_reset(); }

        _rx.active    = true;
        _rx.device_id = h.device_id;
        _rx.msg_id    = h.msg_id;
        _rx.type      = h.type;
        _rx.pkt_count = 0;
        _rx.buf_pos   = 0;

        if (h.type == TYPE_VOICE) {
            _rx.total      = (uint16_t)pld[0] | ((uint16_t)pld[1] << 8);
            _rx.adpcm_pred = (int16_t)((uint16_t)pld[2] | ((uint16_t)pld[3] << 8));
            _rx.adpcm_step = (int8_t)pld[4];
            _rx.buf_size   = (size_t)_rx.total * VOICE_SN_DATA + VOICE_S0_DATA + 3;
            _rx.buf        = (uint8_t*)heap_caps_malloc(_rx.buf_size, MALLOC_CAP_SPIRAM);
            if (!_rx.buf) { PDA_LOGE(TAG, "rx: ps_malloc fail"); _rx_reset(); return; }
            _rx.buf_pos    = 3;
            _rx_append_voice(pld + VOICE_S0_META, VOICE_S0_DATA);

        } else if (h.type == TYPE_TEXT) {
            _rx.total    = (uint16_t)pld[0] | ((uint16_t)pld[1] << 8);
            _rx.buf_size = _rx.total + 1;
            _rx.buf      = (uint8_t*)malloc(_rx.buf_size);
            if (!_rx.buf) { PDA_LOGE(TAG, "rx: malloc fail"); _rx_reset(); return; }
            _rx.buf_pos  = 0;
            _rx_append_text(pld + TEXT_S0_META, TEXT_S0_DATA);
        }

        _rx.pkt_count++;
        return;
    }

    // ── seq>0: продолжение ────────────────────────────────
    if (!_rx.active || _rx.device_id != h.device_id || _rx.msg_id != h.msg_id) return;
    if (h.seq != _rx.pkt_count) {
        PDA_LOGW(TAG, "rx: seq gap %u vs %u", h.seq, _rx.pkt_count);
        _rx_reset();
        return;
    }
    if      (_rx.type == TYPE_VOICE) _rx_append_voice(pld, VOICE_SN_DATA);
    else if (_rx.type == TYPE_TEXT)  _rx_append_text(pld, TEXT_SN_DATA);
    _rx.pkt_count++;
    if (_rx.pkt_count >= _rx.total) _rx_flush();
}

void ChatApp::_rx_append_voice(const uint8_t* adpcm, size_t len) {
    if (!_rx.buf) return;
    size_t space = _rx.buf_size - _rx.buf_pos;
    if (len > space) len = space;
    memcpy(_rx.buf + _rx.buf_pos, adpcm, len);
    _rx.buf_pos += len;
}

void ChatApp::_rx_append_text(const uint8_t* text, size_t len) {
    if (!_rx.buf) return;
    size_t space = _rx.buf_size - 1 - _rx.buf_pos;
    if (len > space) len = space;
    memcpy(_rx.buf + _rx.buf_pos, text, len);
    _rx.buf_pos += len;
}

void ChatApp::_rx_flush() {
    if (!_rx.active || !_rx.buf) { _rx_reset(); return; }

    char path[32];
    char preview[28] = {};

    if (_rx.type == ChatProtocol::TYPE_VOICE) {
        _rx.buf[0] = (uint8_t)(_rx.adpcm_pred & 0xFF);
        _rx.buf[1] = (uint8_t)(_rx.adpcm_pred >> 8);
        _rx.buf[2] = (uint8_t)_rx.adpcm_step;
        snprintf(path, sizeof(path), "/chat/%02X/v_%02X.adpcm",
                 _rx.device_id, _rx.msg_id);
        PDA.Fs.internal.writeBytes(path, _rx.buf, _rx.buf_pos);

        // Оценка длительности
        uint32_t bytes = (uint32_t)ChatProtocol::VOICE_S0_DATA
                       + ((uint32_t)_rx.total - 1) * (uint32_t)ChatProtocol::VOICE_SN_DATA;
        uint32_t secs  = (bytes * 2) / 8000;
        snprintf(preview, sizeof(preview), "> %u:%02u", secs / 60, secs % 60);

        _add_bubble(_rx.device_id, _rx.msg_id, _rx.type, false, preview);
        _send_read(_rx.msg_id);

    } else if (_rx.type == ChatProtocol::TYPE_TEXT) {
        _rx.buf[_rx.buf_pos] = 0;
        snprintf(path, sizeof(path), "/chat/%02X/t_%02X.txt",
                 _rx.device_id, _rx.msg_id);
        PDA.Fs.internal.writeBytes(path, _rx.buf, _rx.buf_pos);

        strncpy(preview, (const char*)_rx.buf, 27);
        preview[27] = 0;

        _add_bubble(_rx.device_id, _rx.msg_id, _rx.type, false, preview);
        _send_read(_rx.msg_id);
    }

    PDA_LOGI(TAG, "rx: saved %s (%u bytes)", path, (unsigned)_rx.buf_pos);
    _rx_reset();
    _update_header();
}

void ChatApp::_rx_reset() {
    if (_rx.buf) {
        if (_rx.type == ChatProtocol::TYPE_VOICE) heap_caps_free(_rx.buf);
        else                                       free(_rx.buf);
        _rx.buf = nullptr;
    }
    _rx = RxState{};
}

// ══════════════════════════════════════════════════════════
//  TX
// ══════════════════════════════════════════════════════════

void ChatApp::_tx_begin_voice() {
    if (_tx.active || !_rec_buf || _rec_buf_pos <= 3) return;

    size_t   data_len = _rec_buf_pos - 3;
    uint16_t total    = 1;
    if (data_len > ChatProtocol::VOICE_S0_DATA) {
        size_t rest = data_len - ChatProtocol::VOICE_S0_DATA;
        total += (uint16_t)((rest + ChatProtocol::VOICE_SN_DATA - 1)
                            / ChatProtocol::VOICE_SN_DATA);
    }
    _tx.active     = true;
    _tx.type       = ChatProtocol::TYPE_VOICE;
    _tx.msg_id     = _next_msg_id++;
    _tx.buf        = _rec_buf + 3;
    _tx.buf_len    = data_len;
    _tx.buf_pos    = 0;
    _tx.seq        = 0;
    _tx.total_pkts = total;
    _tx.adpcm_pred = _rec_initial_pred;
    _tx.adpcm_step = (int8_t)_rec_initial_step;
    PDA_LOGI(TAG, "tx: voice msg=0x%02X pkts=%u bytes=%u",
             _tx.msg_id, total, (unsigned)data_len);
}

void ChatApp::_tx_begin_text() {
    if (_tx.active || _text_len == 0) return;
    _tx.active     = true;
    _tx.type       = ChatProtocol::TYPE_TEXT;
    _tx.msg_id     = _next_msg_id++;
    _tx.buf        = (const uint8_t*)_text_buf;
    _tx.buf_len    = _text_len;
    _tx.buf_pos    = 0;
    _tx.seq        = 0;
    _tx.total_pkts = 0;
    PDA_LOGI(TAG, "tx: text msg=0x%02X bytes=%u", _tx.msg_id, (unsigned)_text_len);
    _update_header();
}

void ChatApp::_tx_step() {
    using namespace ChatProtocol;
    while (_tx.active) {
        uint8_t pkt[PKT_SIZE] = {};
        Header h;
        h.type      = _tx.type;
        h.device_id = _own_id;
        h.msg_id    = _tx.msg_id;
        h.seq       = _tx.seq;
        pack_header(pkt, h);
        uint8_t* pld = pkt + HDR_SIZE;

        if (_tx.type == TYPE_VOICE) {
            if (_tx.seq == 0) {
                pld[0] = (uint8_t)(_tx.total_pkts & 0xFF);
                pld[1] = (uint8_t)(_tx.total_pkts >> 8);
                pld[2] = (uint8_t)(_tx.adpcm_pred & 0xFF);
                pld[3] = (uint8_t)(_tx.adpcm_pred >> 8);
                pld[4] = (uint8_t)_tx.adpcm_step;
                size_t copy = VOICE_S0_DATA;
                if (_tx.buf_pos + copy > _tx.buf_len) copy = _tx.buf_len - _tx.buf_pos;
                memcpy(pld + VOICE_S0_META, _tx.buf + _tx.buf_pos, copy);
                _tx.buf_pos += copy;
            } else {
                size_t copy = VOICE_SN_DATA;
                if (_tx.buf_pos + copy > _tx.buf_len) copy = _tx.buf_len - _tx.buf_pos;
                memcpy(pld, _tx.buf + _tx.buf_pos, copy);
                _tx.buf_pos += copy;
            }
        } else if (_tx.type == TYPE_TEXT) {
            if (_tx.seq == 0) {
                pld[0] = (uint8_t)(_tx.buf_len & 0xFF);
                pld[1] = (uint8_t)(_tx.buf_len >> 8);
                size_t copy = TEXT_S0_DATA;
                if (_tx.buf_pos + copy > _tx.buf_len) copy = _tx.buf_len - _tx.buf_pos;
                memcpy(pld + TEXT_S0_META, _tx.buf + _tx.buf_pos, copy);
                _tx.buf_pos += copy;
            } else {
                size_t copy = TEXT_SN_DATA;
                if (_tx.buf_pos + copy > _tx.buf_len) copy = _tx.buf_len - _tx.buf_pos;
                memcpy(pld, _tx.buf + _tx.buf_pos, copy);
                _tx.buf_pos += copy;
            }
        }

        if (!PDA.Nrf.sendPacket(pkt, PKT_SIZE)) break;  // очередь Nrf полна

        _tx.seq++;
        if (_tx.buf_pos >= _tx.buf_len) { _tx_finish(); break; }
    }
}

void ChatApp::_tx_finish() {
    using namespace ChatProtocol;

    uint8_t pkt[PKT_SIZE] = {};
    Header h;
    h.type      = TYPE_END;
    h.device_id = _own_id;
    h.msg_id    = _tx.msg_id;
    h.seq       = _tx.seq;
    pack_header(pkt, h);
    PDA.Nrf.sendPacket(pkt, PKT_SIZE);

    char path[32];
    char preview[28] = {};

    if (_tx.type == TYPE_VOICE) {
        snprintf(path, sizeof(path), "/chat/%02X/v_%02X.adpcm", _own_id, _tx.msg_id);
        PDA.Fs.internal.writeBytes(path, _rec_buf, _rec_buf_pos);

        uint32_t bytes = (uint32_t)VOICE_S0_DATA
                       + ((uint32_t)_tx.total_pkts - 1) * (uint32_t)VOICE_SN_DATA;
        uint32_t secs  = (bytes * 2) / 8000;
        snprintf(preview, sizeof(preview), "> %u:%02u", secs / 60, secs % 60);

        _add_bubble(_own_id, _tx.msg_id, _tx.type, true, preview);

        heap_caps_free(_rec_buf);
        _rec_buf     = nullptr;
        _rec_buf_pos = 3;

    } else if (_tx.type == TYPE_TEXT) {
        snprintf(path, sizeof(path), "/chat/%02X/t_%02X.txt", _own_id, _tx.msg_id);
        PDA.Fs.internal.writeBytes(path, (const uint8_t*)_text_buf, _text_len);

        strncpy(preview, _text_buf, 27);
        preview[27] = 0;

        _add_bubble(_own_id, _tx.msg_id, _tx.type, true, preview);

        _text_len    = 0;
        _text_buf[0] = 0;
    }

    PDA_LOGI(TAG, "tx: done msg=0x%02X pkts=%u -> %s",
             _tx.msg_id, _tx.seq, path);
    _tx = TxState{};
    _update_header();
}

// ══════════════════════════════════════════════════════════
//  FS
// ══════════════════════════════════════════════════════════

void ChatApp::_ensure_dir(uint8_t device_id) {
    if (!PDA.Fs.internal.exists("/chat"))
        PDA.Fs.internal.mkdir("/chat");
    char path[16];
    snprintf(path, sizeof(path), "/chat/%02X", device_id);
    if (!PDA.Fs.internal.exists(path))
        PDA.Fs.internal.mkdir(path);
}

// ══════════════════════════════════════════════════════════
//  Audio
// ══════════════════════════════════════════════════════════

void ChatApp::_startRecord() {
    if (_recording || _tx.active || _playing) return;
    if (!_rec_buf) {
        _rec_buf = (uint8_t*)heap_caps_malloc(REC_MAX + 3, MALLOC_CAP_SPIRAM);
        if (!_rec_buf) { PDA_LOGE(TAG, "rec: malloc fail"); return; }
    }
    _rec_buf_pos     = 3;
    _first_rec_frame = true;
    _recording       = true;
    _audio.startCapture();
    _send_status(ChatProtocol::STATUS_RECORDING);
    _update_header();
}

void ChatApp::_stopRecord() {
    if (!_recording) return;
    _recording = false;
    _audio.stopCapture();
    _send_status(ChatProtocol::STATUS_IDLE);

    if (_rec_buf) {
        Walkie::TxFrame tf;
        while (xQueueReceive(_tx_q, &tf, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (_first_rec_frame) {
                _rec_initial_pred = tf.pred;
                _rec_initial_step = tf.step_idx;
                _first_rec_frame  = false;
            }
            size_t chunk = sizeof(tf.data);
            if (_rec_buf_pos + chunk > REC_MAX + 3) break;
            memcpy(_rec_buf + _rec_buf_pos, tf.data, chunk);
            _rec_buf_pos += chunk;
        }
    }

    if (_rec_buf_pos <= 3) { PDA_LOGW(TAG, "rec: empty"); _update_header(); return; }

    _rec_buf[0] = (uint8_t)(_rec_initial_pred & 0xFF);
    _rec_buf[1] = (uint8_t)(_rec_initial_pred >> 8);
    _rec_buf[2] = _rec_initial_step;
    _tx_begin_voice();
}

void ChatApp::_startPlay(uint8_t device_id, uint8_t msg_id) {
    if (_recording || _playing) return;

    char path[32];
    snprintf(path, sizeof(path), "/chat/%02X/v_%02X.adpcm", device_id, msg_id);
    if (!PDA.Fs.internal.exists(path)) {
        PDA_LOGW(TAG, "play: not found %s", path);
        return;
    }

    size_t file_size = PDA.Fs.internal.size(path);
    if (file_size < 3) return;

    uint8_t* buf = (uint8_t*)heap_caps_malloc(file_size, MALLOC_CAP_SPIRAM);
    if (!buf) { PDA_LOGE(TAG, "play: malloc fail"); return; }
    if (!PDA.Fs.internal.readBytes(path, buf, file_size)) {
        heap_caps_free(buf); return;
    }

    _play_buf         = buf;
    _play_buf_size    = file_size;
    _play_adpcm_pos   = 0;
    _play_state.pred  = (int16_t)((uint16_t)buf[0] | ((uint16_t)buf[1] << 8));
    _play_state.index = (int8_t)buf[2];
    _playing          = true;

    PDA_LOGI(TAG, "play: %s (%u bytes)", path, (unsigned)file_size);
    _update_header();
}

void ChatApp::_stopPlay() {
    if (!_playing) return;
    _playing = false;
    if (_play_buf) { heap_caps_free(_play_buf); _play_buf = nullptr; }
    _play_buf_size  = 0;
    _play_adpcm_pos = 0;
    _update_header();
}

// ══════════════════════════════════════════════════════════
//  UI helpers
// ══════════════════════════════════════════════════════════

void ChatApp::_update_header() {
    if (!_lbl_header) return;
    char buf[32];
    if (_recording) {
        snprintf(buf, sizeof(buf), "Chat  o REC");
    } else if (_playing) {
        snprintf(buf, sizeof(buf), "Chat  > play");
    } else if (_tx.active) {
        snprintf(buf, sizeof(buf), "Chat  >> TX");
    } else {
        switch (_peer_status) {
            case ChatProtocol::STATUS_TYPING:
                snprintf(buf, sizeof(buf), "Chat  пишет...");
                break;
            case ChatProtocol::STATUS_RECORDING:
                snprintf(buf, sizeof(buf), "Chat  записывает");
                break;
            default:
#ifdef PDA2_LITE
                snprintf(buf, sizeof(buf), _lang_ru ? "Chat [RU]" : "Chat [EN]");
#else
                snprintf(buf, sizeof(buf), "Chat");
#endif
                break;
        }
    }
    lv_label_set_text(_lbl_header, buf);
}

void ChatApp::_update_input_label() {
    if (!_lbl_input) return;
#ifdef PDA2_LITE
    constexpr size_t TAIL = 18;
    char buf[26];
#else
    constexpr size_t TAIL = 34;
    char buf[42];
#endif
    const char* ptr = _text_buf;
    size_t      len = _text_len;
    if (len > TAIL) {
        ptr = _text_buf + (len - TAIL);
        // Выравниваем по границе UTF-8
        while (ptr < _text_buf + len && (*ptr & 0xC0) == 0x80) ptr++;
    }
    bool show_cur = _input_active && _cursor_visible;
    snprintf(buf, sizeof(buf), "%s%c", ptr, show_cur ? '_' : ' ');
    lv_label_set_text(_lbl_input, buf);
}

void ChatApp::_add_bubble(uint8_t device_id, uint8_t msg_id, uint8_t type,
                           bool outgoing, const char* preview) {
    if (_msg_count >= MAX_MSGS) return;

    MsgEntry& e = _msgs[_msg_count];
    e.device_id = device_id;
    e.msg_id    = msg_id;
    e.type      = type;
    e.outgoing  = outgoing;
    e.read      = false;
    strncpy(e.preview, preview, sizeof(e.preview) - 1);
    e.preview[sizeof(e.preview) - 1] = 0;

    // Пузырь в flex-контейнере
    lv_obj_t* bub = lv_obj_create(_list_cont);
    lv_obj_set_width(bub, CHAT_BUB_W);
    lv_obj_set_height(bub, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(bub, outgoing ? COL_OUT_BUB : COL_IN_BUB, 0);
    lv_obj_set_style_bg_opa(bub, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bub, 0, 0);
    lv_obj_set_style_radius(bub, 4, 0);
    lv_obj_set_style_pad_all(bub, 3, 0);
    // Входящие — влево (margin 2), исходящие — вправо (margin = SCR_W - BUB_W - 4)
    lv_obj_set_style_margin_left(bub,
        outgoing ? (lv_coord_t)(CHAT_SCR_W - CHAT_BUB_W - 4) : 2, 0);

    // Текст + галочка в одном лейбле (ПРАВИЛО 10)
    lv_obj_t* lbl = lv_label_create(bub);
    lv_obj_set_style_text_font(lbl, PDA.Fonts.get(12), 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_bg_color(lbl, outgoing ? COL_OUT_BUB : COL_IN_BUB, 0);
    lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);          // ПРАВИЛО 10
    lv_obj_set_width(lbl, CHAT_BUB_W - 6);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);

    char display[34] = {};
    if (outgoing)
        snprintf(display, sizeof(display), "%s v", preview);
    else
        snprintf(display, sizeof(display), "%s",   preview);
    lv_label_set_text(lbl, display);

#ifndef PDA2_LITE
    if (type == ChatProtocol::TYPE_VOICE) {
        lv_obj_add_event_cb(bub, _cb_bubble, LV_EVENT_CLICKED,
                            (void*)(uintptr_t)_msg_count);
    }
#endif

    e.bubble   = bub;
    e.lbl_text = lbl;
    _msg_count++;

    lv_obj_scroll_to_y(_list_cont, LV_COORD_MAX, LV_ANIM_OFF);
}

// ══════════════════════════════════════════════════════════
//  _set_focus
// ══════════════════════════════════════════════════════════

void ChatApp::_set_focus(int8_t idx) {
    // Снять подсветку с предыдущего
    if (!_input_active && _focus_idx >= 0 && _focus_idx < (int8_t)_msg_count)
        lv_obj_set_style_border_width(_msgs[_focus_idx].bubble, 0, 0);

    if (idx < 0) {
        _focus_idx    = -1;
        _input_active = true;
    } else {
        _focus_idx    = idx;
        _input_active = false;
        lv_obj_set_style_border_width(_msgs[idx].bubble, 2, 0);
        lv_obj_set_style_border_color(_msgs[idx].bubble, COL_FOCUS, 0);
        lv_obj_scroll_to_view(_msgs[idx].bubble, LV_ANIM_OFF);
    }
    _update_input_label();  // курсор — только если _input_active
}

// ══════════════════════════════════════════════════════════
//  _send_status / _send_read / _mark_read
// ══════════════════════════════════════════════════════════

void ChatApp::_send_status(uint8_t status) {
    using namespace ChatProtocol;
    uint8_t pkt[PKT_SIZE] = {};
    Header h;
    h.type      = TYPE_STATUS;
    h.device_id = _own_id;
    h.msg_id    = 0;
    h.seq       = 0;
    pack_header(pkt, h);
    pkt[HDR_SIZE] = status;
    PDA.Nrf.sendPacket(pkt, PKT_SIZE);
}

void ChatApp::_send_read(uint8_t msg_id) {
    using namespace ChatProtocol;
    uint8_t pkt[PKT_SIZE] = {};
    Header h;
    h.type      = TYPE_READ;
    h.device_id = _own_id;
    h.msg_id    = msg_id;
    h.seq       = 0;
    pack_header(pkt, h);
    PDA.Nrf.sendPacket(pkt, PKT_SIZE);
}

void ChatApp::_mark_read(uint8_t msg_id) {
    for (uint8_t i = 0; i < _msg_count; i++) {
        if (_msgs[i].outgoing && _msgs[i].msg_id == msg_id && !_msgs[i].read) {
            _msgs[i].read = true;
            if (_msgs[i].lbl_text) {
                char display[34];
                snprintf(display, sizeof(display), "%s vv", _msgs[i].preview);
                lv_label_set_text(_msgs[i].lbl_text, display);
            }
            break;
        }
    }
}

*/