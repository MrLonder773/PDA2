// src/apps/chat/ChatApp.h — PDA 2 / PDA 2 Lite (128×160 ST7735S | 320×480)
/*
#pragma once
#include <PDA2.h>
#include "chat_protocol.h"
#include "adpcm.h"
#include "WalkieAudio.h"
#include "walkie_types.h"
// ════════════════════════════════════════════════════════
//  ChatApp  — Текст + голос через nRF24.
//
//  LittleFS:
//    /chat/<device_id>/v_<msg_id>.adpcm  [pred:2][step:1][adpcm...]
//    /chat/<device_id>/t_<msg_id>.txt
// ════════════════════════════════════════════════════════

class ChatApp : public PDA2App {
public:
    const char* name = "Chat";
    explicit ChatApp(uint8_t own_id = 0x01, uint8_t peer_id = 0x02);

    void onInit()                                 override;
    void onOpen()                                 override;
    void onClose()                                override;
    void onTick(uint32_t delta_ms)                override;
    #ifndef PDA2_LITE
    void onButton(uint8_t clicks) override;
    #endif
    void onKey(uint8_t keycode, uint8_t modifier) override;

private:
    // ── IDs ──────────────────────────────────────────────
    uint8_t _own_id;
    uint8_t _peer_id;
    uint8_t _next_msg_id = 0;

    // ── RX: сборка входящего сообщения ───────────────────
    struct RxState {
        bool     active     = false;
        uint8_t  device_id  = 0;
        uint8_t  msg_id     = 0;
        uint8_t  type       = 0;
        uint16_t total      = 0;       // total_pkts (voice) / total_bytes (text)
        uint16_t pkt_count  = 0;
        uint8_t* buf        = nullptr;
        size_t   buf_pos    = 0;
        size_t   buf_size   = 0;
        int16_t  adpcm_pred = 0;
        int8_t   adpcm_step = 0;
    };
    RxState _rx;

    // ── TX ────────────────────────────────────────────────
    struct TxState {
        bool           active      = false;
        uint8_t        type        = 0;
        uint8_t        msg_id      = 0;
        const uint8_t* buf         = nullptr;
        size_t         buf_len     = 0;
        size_t         buf_pos     = 0;
        uint16_t       seq         = 0;
        uint16_t       total_pkts  = 0;
        int16_t        adpcm_pred  = 0;
        int8_t         adpcm_step  = 0;
    };
    TxState _tx;

    // ── Audio ─────────────────────────────────────────────
    WalkieAudio   _audio;
    QueueHandle_t _tx_q = nullptr;
    QueueHandle_t _rx_q = nullptr;

    // ── Запись ────────────────────────────────────────────
    static constexpr size_t REC_MAX = 120 * 1024;
    bool     _recording        = false;
    uint8_t* _rec_buf          = nullptr;
    size_t   _rec_buf_pos      = 3;
    bool     _first_rec_frame  = true;
    int16_t  _rec_initial_pred = 0;
    uint8_t  _rec_initial_step = 0;

    // ── Воспроизведение ───────────────────────────────────
    bool         _playing        = false;
    uint8_t*     _play_buf       = nullptr;
    size_t       _play_buf_size  = 0;
    size_t       _play_adpcm_pos = 0;
    ADPCM::State _play_state;

    // ── Текстовый буфер ───────────────────────────────────
    char   _text_buf[512] = {};
    size_t _text_len      = 0;

    bool _lang_ru = false;   // EN / RU toggle
    bool _nrf_ready = false;
    // ── UI: размеры ───────────────────────────────────────
#ifdef PDA2_LITE
    static constexpr uint16_t CHAT_SCR_W  = 128;
    static constexpr uint16_t CHAT_SCR_H  = 160;
    static constexpr uint16_t CHAT_HDR_H  = 16;
    static constexpr uint16_t CHAT_BTN_H  = 20;
    static constexpr uint16_t CHAT_INP_H  = 20;
    static constexpr uint16_t CHAT_BUB_W  = 100;
#else
    static constexpr uint16_t CHAT_SCR_W  = 320;
    static constexpr uint16_t CHAT_SCR_H  = 480;
    static constexpr uint16_t CHAT_HDR_H  = 44;
    static constexpr uint16_t CHAT_BTN_H  = 64;
    static constexpr uint16_t CHAT_INP_H  = 56;
    static constexpr uint16_t CHAT_BUB_W  = 220;
#endif
    static constexpr uint16_t CHAT_LIST_H =
        CHAT_SCR_H - CHAT_HDR_H - CHAT_INP_H - CHAT_BTN_H;

    // ── UI: объекты (общие) ───────────────────────────────
    lv_obj_t* _lbl_header = nullptr;
    lv_obj_t* _list_cont  = nullptr;
    lv_obj_t* _lbl_input  = nullptr;

#ifndef PDA2_LITE
    // Тач-объекты (только main)
    lv_obj_t* _lbl_lang   = nullptr;   // [EN] / [RU] в header
    lv_obj_t* _btn_ptt    = nullptr;
    lv_obj_t* _lbl_ptt    = nullptr;
    lv_obj_t* _btn_send   = nullptr;

    static void _cb_ptt   (lv_event_t* e);
    static void _cb_send  (lv_event_t* e);
    static void _cb_lang  (lv_event_t* e);
    static void _cb_back  (lv_event_t* e);
    static void _cb_bubble(lv_event_t* e);
#endif

    // ── Список сообщений ──────────────────────────────────
    struct MsgEntry {
        uint8_t   device_id;
        uint8_t   msg_id;
        uint8_t   type;        // TYPE_TEXT / TYPE_VOICE
        bool      outgoing;
        bool      read;
        lv_obj_t* bubble;      // контейнер пузыря (для фокуса)
        lv_obj_t* lbl_text;    // текст + " v" / " vv" (ПРАВИЛО 10)
        char      preview[28]; // чистый текст без галочек (для обновления)
    };
    static constexpr uint8_t MAX_MSGS = 20;
    MsgEntry _msgs[MAX_MSGS]  = {};
    uint8_t  _msg_count       = 0;

    // ── Навигация ─────────────────────────────────────────
    int8_t _focus_idx    = -1;    // -1 = поле ввода
    bool   _input_active = true;

    // ── Статус peer-а ─────────────────────────────────────
    uint8_t  _peer_status     = ChatProtocol::STATUS_IDLE;
    uint32_t _peer_status_age = 0;   // мс с последнего TYPE_STATUS

    // ── Таймеры ───────────────────────────────────────────
    uint32_t _typing_timer    = 0;
    uint32_t _cursor_blink_ms = 0;
    bool     _cursor_visible  = true;

    // ── Nrf колбэк ────────────────────────────────────────
    static void _on_packet(const uint8_t* data, uint8_t len);
    void _process_packet(const uint8_t* data, uint8_t len);
    static void _on_nrf_push(uint8_t cmd, const uint8_t* data, uint8_t len);
    // ── RX ────────────────────────────────────────────────
    void _rx_reset();
    void _rx_append_voice(const uint8_t* adpcm, size_t len);
    void _rx_append_text(const uint8_t* text, size_t len);
    void _rx_flush();

    // ── TX ────────────────────────────────────────────────
    void _tx_begin_voice();
    void _tx_begin_text();
    void _tx_step();
    void _tx_finish();

    // ── FS ────────────────────────────────────────────────
    void _ensure_dir(uint8_t device_id);

    // ── Audio ─────────────────────────────────────────────
    void _startRecord();
    void _stopRecord();
    void _startPlay(uint8_t device_id, uint8_t msg_id);
    void _stopPlay();

    // ── UI helpers ────────────────────────────────────────
    void _update_header();
    void _update_input_label();
    void _add_bubble(uint8_t device_id, uint8_t msg_id, uint8_t type,
                     bool outgoing, const char* preview);
    void _set_focus(int8_t idx);
    void _send_status(uint8_t status);
    void _send_read(uint8_t msg_id);
    void _mark_read(uint8_t msg_id);

    static ChatApp* _s_this;
};

*/