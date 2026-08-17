// ════════════════════════════════════════════════════════
//  PDA 2 — Usb_Class.cpp
//  Общая часть — кольцевой буфер, колбэки, keycodeToChar.
//  Полностью переносима между ESP32 и PC.
//
//  begin() и приём событий с устройств — платформозависимы,
//  см. Usb_Class_esp32.cpp / Usb_Class_sim.cpp.
// ════════════════════════════════════════════════════════

#include "Usb_Class.h"
#include "../pda2_log.h"

// ════════════════════════════════════════════════════════
//  Usb_Class — общая реализация
// ════════════════════════════════════════════════════════

void pda2::Usb_Class::update() {
    // На PC здесь реально опрашивается SDL и события кладутся
    // в буфер; на ESP32 это пустая функция (события уже пришли
    // асинхронно через колбэки USB-стека).
    _platformPoll(this);

    while (_r != _w) {
        UsbEvent e = _buf[_r];
        _r = (_r + 1) % USB_EVENT_BUF_SIZE;

        if (e.type == UsbEventType::KEY) {
            if (_key_cb)
                _key_cb(e.key.keycode, e.key.modifier, e.key.pressed);
        } else if (e.type == UsbEventType::MOUSE) {
            if (_mouse_cb)
                _mouse_cb(e.mouse.dx, e.mouse.dy, e.mouse.buttons, e.mouse.scroll);
        } else if (e.type == UsbEventType::GAMEPAD) {
            if (_gamepad_cb)
                _gamepad_cb(e.gamepad.state);
        }
    }
}

void pda2::Usb_Class::_push(const UsbEvent& e) {
    uint8_t next = (_w + 1) % USB_EVENT_BUF_SIZE;
    if (next == _r) {
        PDA_LOGW("usb", "event buffer overflow");
        return;
    }
    _buf[_w] = e;
    _w = next;
}

void pda2::Usb_Class::_pushKeyEvent(uint8_t keycode, uint8_t modifier, bool pressed) {
    UsbEvent e;
    e.type         = UsbEventType::KEY;
    e.key.keycode  = keycode;
    e.key.modifier = modifier;
    e.key.pressed  = pressed;
    _push(e);
}

void pda2::Usb_Class::_pushMouseEvent(int8_t dx, int8_t dy, uint8_t buttons, int8_t scroll) {
    UsbEvent e;
    e.type          = UsbEventType::MOUSE;
    e.mouse.dx      = dx;
    e.mouse.dy      = dy;
    e.mouse.buttons = buttons;
    e.mouse.scroll  = scroll;
    _push(e);
}

void pda2::Usb_Class::_pushGamepadEvent(const GamepadState& state) {
    UsbEvent e;
    e.type          = UsbEventType::GAMEPAD;
    e.gamepad.state = state;
    _push(e);
}

char pda2::Usb_Class::keycodeToChar(uint8_t keycode, uint8_t modifier) {
    const bool shift = modifier & 0x22; // LShift | RShift

    if (keycode >= 4 && keycode <= 29) {
        char c = 'a' + (keycode - 4);
        return shift ? (c - 32) : c;
    }

    static const char nums[]    = "1234567890";
    static const char shifted[] = "!@#$%^&*()";
    if (keycode >= 30 && keycode <= 39)
        return shift ? shifted[keycode - 30] : nums[keycode - 30];

    if (!shift) switch (keycode) {
        case 44: return ' ';
        case 45: return '-';
        case 46: return '=';
        case 47: return '[';
        case 48: return ']';
        case 49: return '\\';
        case 51: return ';';
        case 52: return '\'';
        case 53: return '`';
        case 54: return ',';
        case 55: return '.';
        case 56: return '/';
    } else switch (keycode) {
        case 45: return '_';
        case 46: return '+';
        case 47: return '{';
        case 48: return '}';
        case 49: return '|';
        case 51: return ':';
        case 52: return '"';
        case 53: return '~';
        case 54: return '<';
        case 55: return '>';
        case 56: return '?';
    }

    return 0;
}