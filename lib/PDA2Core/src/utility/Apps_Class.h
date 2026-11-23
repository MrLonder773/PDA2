#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — Apps_Class.h
//  Менеджер приложений + launcher screen.
//  Launcher — отдельный lv_screen, строится здесь.
//  Каждое приложение — свой lv_screen (создаётся в onInit).
//
//  Реализация разбита по блокам:
//    Apps_Class.cpp     — ядро: реестр, open/close, tick
//    Apps_Input.cpp     — клавиши (USB/BLE), навигация
//    Apps_Launcher.cpp  — постройка launcher
//    Apps_Toast.cpp     — тост
// ════════════════════════════════════════════════════════

#include "../PDA2App.h"
#include "../pda2_config.h"
#include <lvgl.h>

class Apps_Class {
public:
    // Регистрация. Вызывать до start().
    void add(PDA2App* app);

    // Вызвать после всех add() — строит launcher и вызывает onInit().
    void start();

    // Открыть приложение по индексу (анимация MOVE_LEFT).
    void open(uint8_t id);

    // ── Navigation (keyboard / BLE) ───────────────────────
    void next();
    void prev();
    void openSelected();

    // Закрыть текущее приложение, вернуться в launcher (MOVE_RIGHT).
    void close();

    // Toast — вызывается через PDA.toast()
    void showToast(const char* text, uint32_t ms = 2000);

    // Единый колбэк для PDA.Usb.onKey() и PDA.Bt.onKey().
    // Регистрируется в PDA2.cpp::begin(), не трогать в main.cpp.
    static void _key_cb(uint8_t keycode, uint8_t modifier, bool pressed);

    // Вызывается из PDA2Class::update() каждый фрейм.
    void tick(uint32_t delta_ms);

    PDA2App* current();
    bool     isOpen();

    uint8_t  count();

private:
    // Экземпляр для static-колбэков (задаётся в start()), общий для всех блоков
    static Apps_Class* _instance;

    PDA2App* _apps[PDA2_MAX_APPS] = {};
    uint8_t   _count       = 0;
    int8_t    _current_id  = -1;   // -1 = launcher активен
    bool      _is_open     = false;

    lv_obj_t* _screen_launcher = nullptr;

    int8_t     _page  = 0;               // страница под пальцем (по смещению прокрутки)
    int8_t     _pages = 1;               // число страниц (считает _build_launcher)
    lv_obj_t*  _pager = nullptr;         // горизонтальный контейнер со страницами
    lv_obj_t*  _dots[PDA2_MAX_APPS] = {};

    int8_t     _selected_id = 0;
    lv_obj_t*  _icon_btns[PDA2_MAX_APPS] = {};        // (для подсветки)

    // Toast
    lv_obj_t* _toast_label  = nullptr;
    lv_obj_t* _toast_cont   = nullptr;
    uint32_t  _toast_until  = 0;

    void _build_launcher();
    void _build_toast();
    void _tick_toast(uint32_t now);

    void _updateLauncherSelection();

    void _update_dots();
    void _select(int8_t id);             // выбор с прокруткой к нужной странице
    static void _scroll_cb(lv_event_t* e);
    static void _scroll_end_cb(lv_event_t* e);

    // Callback кнопок launcher
    static void _btn_cb(lv_event_t* e);

    // Callback удаления app-экрана (LV_EVENT_DELETE) — обнуляет PDA2App::screen
    static void _screen_del_cb(lv_event_t* e);
};