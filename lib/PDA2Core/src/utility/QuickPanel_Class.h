#pragma once
#include <lvgl.h>

// ═══════════════════════════════════════════════════════════
//  QuickPanel_Class
//  Две шторки поверх всего (lv_layer_top):
//    — свайп из левой  половины → уведомления
//    — свайп из правой половины → быстрые настройки
// ═══════════════════════════════════════════════════════════

class QuickPanel_Class {
public:
    void begin();

    void openNotif();       // открыть панель уведомлений
    void openSettings();    // открыть панель настроек
    void close();           // закрыть активную панель
    bool isOpen() const { return _open; }

    // API уведомлений — вызывается через PDA.notify()
    void pushNotif(const char* app_name, const char* text);
    void clearNotifs();

    // вызывается из static callback анимации
    void _onCloseDone();

private:
    // ── Слои ────────────────────────────────────────────────
    lv_obj_t* _overlay         = nullptr;  // затемнение фона
    lv_obj_t* _panel_notif     = nullptr;
    lv_obj_t* _panel_settings  = nullptr;

    // Элементы панели уведомлений
    lv_obj_t* _cont_list       = nullptr;  // контейнер карточек
    lv_obj_t* _lbl_empty       = nullptr;

    // Элементы панели настроек
    lv_obj_t* _slider_br       = nullptr;
    lv_obj_t* _lbl_br_val      = nullptr;
    lv_obj_t* _sw_wifi         = nullptr;
    lv_obj_t* _lbl_wifi        = nullptr;

    // ── Состояние ───────────────────────────────────────────
    bool      _open                  = false;
    bool      _ignore_overlay_click  = false;
    lv_obj_t* _active_panel          = nullptr;
    int32_t   _active_h              = 0;

    // ── Хранилище уведомлений ───────────────────────────────
    struct Notif {
        char    app_name[16];
        char    text[64];
        uint8_t hour;
        uint8_t minute;
        bool    used = false;
    };
    Notif   _notifs[8];   // PDA2_NOTIF_MAX
    uint8_t _notif_head  = 0;
    uint8_t _notif_count = 0;

    // ── Приватные методы ────────────────────────────────────
    void _build_overlay();
    void _build_notif_panel();
    void _build_settings_panel();
    void _open_panel(lv_obj_t* panel, int32_t h);
    void _refresh_notif_list();

    // ── Callbacks (static → экземпляр через user_data) ─────
    static void _cb_overlay_click(lv_event_t* e);
    static void _cb_slider(lv_event_t* e);
    static void _cb_wifi(lv_event_t* e);
    static void _cb_notif_click(lv_event_t* e);
    static void _cb_anim_close_done(lv_anim_t* a);
};