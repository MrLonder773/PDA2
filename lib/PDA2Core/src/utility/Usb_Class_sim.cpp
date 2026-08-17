// ════════════════════════════════════════════════════════
//  PDA 2 — Usb_Class_sim.cpp
//  PC-реализация: опрос клавиатуры SDL2 раз в кадр (poll,
//  не push, в отличие от ESP32).
//
//  SDL_Scancode специально спроектирован так, что совпадает
//  с USB HID keycode для подавляющего большинства клавиш —
//  отдельная таблица перевода не нужна, достаточно прямого
//  приведения типа.
//
//  Мышь на PC уже занята под тач (см. pda2_platform_touch_read
//  в pda2_platform_sim.cpp) — onMouse() в симуляторе не
//  вызывается. Геймпад не эмулируется.
//
//  Компилируется, только если PDA2_SIM определён —
//  в прошивке ESP32 тело пустое (см. Usb_Class_esp32.cpp).
// ════════════════════════════════════════════════════════

#include "Usb_Class.h"
#include "../pda2_log.h"
#include "../pda2_platform.h"

#ifdef PDA2_SIM

#include <SDL2/SDL.h>

// ── Модификаторы: SDL_Keymod → USB HID modifier byte ────
static uint8_t _sdl_mod_to_hid(SDL_Keymod m) {
    uint8_t r = 0;
    if (m & KMOD_LCTRL)  r |= 0x01;
    if (m & KMOD_LSHIFT) r |= 0x02;
    if (m & KMOD_LALT)   r |= 0x04;
    if (m & KMOD_LGUI)   r |= 0x08;
    if (m & KMOD_RCTRL)  r |= 0x10;
    if (m & KMOD_RSHIFT) r |= 0x20;
    if (m & KMOD_RALT)   r |= 0x40;
    if (m & KMOD_RGUI)   r |= 0x80;
    return r;
}

// ── Сами клавиши-модификаторы (HID usage 224–231) — не
//    отправляем как отдельные keycode-события, ровно как на
//    реальной клавиатуре: они входят только в modifier byte.
static bool _is_modifier_scancode(SDL_Scancode sc) {
    return sc >= SDL_SCANCODE_LCTRL && sc <= SDL_SCANCODE_RGUI;
}

void pda2::Usb_Class::begin() {
    // На PC клавиатура «подключена» всегда.
    _setConnected(true);
    PDA_LOGI("usb", "SIM: keyboard ready (SDL2)");
}

void pda2::Usb_Class::_platformPoll(Usb_Class* self) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            pda2_platform_request_quit();
            continue;
        }
        if (ev.type != SDL_KEYDOWN && ev.type != SDL_KEYUP) continue;
        if (ev.key.repeat) continue;   // как на железе — без авто-повтора

        SDL_Scancode sc = ev.key.keysym.scancode;
        if (_is_modifier_scancode(sc)) continue;

        uint8_t keycode  = (uint8_t)sc;
        uint8_t modifier = _sdl_mod_to_hid(SDL_GetModState());
        bool    pressed  = (ev.type == SDL_KEYDOWN);

        self->_pushKeyEvent(keycode, modifier, pressed);
    }
}

#endif // PDA2_SIM