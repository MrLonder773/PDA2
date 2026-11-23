// ════════════════════════════════════════════════════════
//  PDA 2 — Apps_Class.cpp
//  Ядро: реестр приложений, open/close, tick.
// ════════════════════════════════════════════════════════

#include <PDA2.h>
#include "Apps_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"
#include "../pda2_platform.h"

Apps_Class* Apps_Class::_instance = nullptr;

// ── add / start ──────────────────────────────────────────
void Apps_Class::add(PDA2App* app) {
    if (_count >= PDA2_MAX_APPS) {
        PDA_LOGE("apps", "Max apps reached (%d)", PDA2_MAX_APPS);
        return;
    }
    _apps[_count++] = app;
    PDA_LOGI("apps", "Registered: %s [%d]", app->name, _count - 1);
}

void Apps_Class::start() {
    _instance = this;

    for (uint8_t i = 0; i < _count; i++) {
        if (_apps[i]) {
            _apps[i]->onInit();   // ⚠️ только init состояния, БЕЗ LVGL
            PDA_LOGI("apps", "onInit: %s", _apps[i]->name);
        }
    }

    _build_launcher();
    _build_toast();
    lv_screen_load(_screen_launcher);

    PDA_LOGI("apps", "Launcher ready. %d app(s) registered.", _count);
}

// ── open / close ─────────────────────────────────────────
void Apps_Class::open(uint8_t id) {
    if (id >= _count || !_apps[id]) {
        PDA_LOGE("apps", "open(%d): invalid id", id);
        return;
    }

    if (_is_open) {
        PDA_LOGI("apps", "open(%d): ignored, app already open", id);
        return;
    }

    lv_obj_t* prev_screen = _apps[id]->screen;

    _apps[id]->onOpen();   // строит screen лениво, если его ещё нет

    if (!_apps[id]->screen) {
        PDA_LOGE("apps", "open(%d): onOpen() did not build screen", id);
        return;
    }

    // Колбэк на удаление вешаем один раз — когда screen реально создан впервые.
    if (_apps[id]->screen != prev_screen) {
        lv_obj_add_event_cb(_apps[id]->screen, _screen_del_cb,
                             LV_EVENT_DELETE, (void*)_apps[id]);
    }

    _current_id = id;
    _is_open    = true;

    lv_screen_load_anim(
        _apps[id]->screen,
        LV_SCR_LOAD_ANIM_FADE_IN,
        PDA2_ANIM_MS, 0, false
    );

    PDA_LOGI("apps", "Opened: %s", _apps[id]->name);
    PDA.Memory.log("app opened");
}

void Apps_Class::close() {
    if (!_is_open || _current_id < 0) return;

    _apps[_current_id]->onClose();   // ⚠️ только состояние, screen не трогать
    PDA_LOGI("apps", "Closed: %s", _apps[_current_id]->name);
    PDA.Memory.log("app closed (screen ещё жив)");
    _current_id = -1;
    _is_open    = false;

    lv_screen_load_anim(
        _screen_launcher,
        LV_SCR_LOAD_ANIM_FADE_IN,
        PDA2_ANIM_MS, 0, true   // auto_del=true — LVGL сам удалит app-экран после анимации
    );
}

// ── screen delete callback ───────────────────────────────
void Apps_Class::_screen_del_cb(lv_event_t* e) {
    PDA2App* app = (PDA2App*)lv_event_get_user_data(e);
    if (app) app->screen = nullptr;   // экран реально удалён LVGL — обнуляем указатель
}

// ── tick ─────────────────────────────────────────────────
void Apps_Class::tick(uint32_t delta_ms) {
    if (_is_open && _current_id >= 0 && _apps[_current_id]) {
        _apps[_current_id]->onTick(delta_ms);
    }

    for (uint8_t i = 0; i < _count; i++) {
        if ((int8_t)i == _current_id) continue;
        if (_apps[i]) _apps[i]->onBackground();
    }

    _tick_toast(pda2_platform_now_ms());
    lv_timer_handler();
}

// ── getters ──────────────────────────────────────────────
PDA2App* Apps_Class::current() {
    if (_current_id >= 0) return _apps[_current_id];
    return nullptr;
}
bool    Apps_Class::isOpen()  { return _is_open; }
uint8_t Apps_Class::count()   { return _count; }