// ════════════════════════════════════════════════════════
//  PDA 2 — Usb_Class.h
//  USB Host подсистема. Клавиатура + мышь + геймпад через OTG порт.
//  Библиотека: esp32beans/ESP32_USB_Host_HID
//  Потокобезопасность: события буферизуются из USB task,
//  колбэки вызываются из main task в Usb.update().
// ════════════════════════════════════════════════════════

#pragma once
#include <stdint.h>

namespace pda2 {

// ── Типы колбэков ─────────────────────────────────────────
typedef void (*usb_key_cb_t)(uint8_t keycode, uint8_t modifier, bool pressed);
typedef void (*usb_mouse_cb_t)(int8_t dx, int8_t dy, uint8_t buttons, int8_t scroll);

struct GamepadState {
    int8_t  lx, ly;        // левый стик  (-128..127, 0=центр)
    int8_t  rx, ry;        // правый стик (-128..127, 0=центр)
    int8_t  dpad_x;        // -1=влево, 0=центр, 1=вправо
    int8_t  dpad_y;        // -1=вверх, 0=центр, 1=вниз
    bool    a, b, x, y;
    bool    l1, r1, l2, r2;
    bool    start, select;
    bool    l3, r3;        // клики стиков
};

typedef void (*usb_gamepad_cb_t)(const GamepadState& state);

// ── Размер кольцевого буфера событий ─────────────────────
static constexpr uint8_t USB_EVENT_BUF_SIZE = 16;

// ── Тип события ──────────────────────────────────────────
enum class UsbEventType : uint8_t { KEY, MOUSE, GAMEPAD };

struct UsbEvent {
    UsbEventType type;
    union {
        struct { uint8_t keycode; uint8_t modifier; bool pressed; } key;
        struct { int8_t dx; int8_t dy; uint8_t buttons; int8_t scroll; } mouse;
        struct { GamepadState state; } gamepad;
    };
};

// ════════════════════════════════════════════════════════
class Usb_Class {
public:
    // ── Жизненный цикл ───────────────────────────────────
    void begin();                  // вызывается из PDA.begin()
    void update();                 // вызывается из PDA.update()

    // ── Состояние ────────────────────────────────────────
    bool ok() const { return _connected; }

    // ── Колбэки клавиатуры ───────────────────────────────
    void onKey(usb_key_cb_t cb)  { _key_cb   = cb; }
    void offKey()                { _key_cb   = nullptr; }

    // ── Колбэки мыши ─────────────────────────────────────
    void onMouse(usb_mouse_cb_t cb) { _mouse_cb = cb; }
    void offMouse()                 { _mouse_cb = nullptr; }

    // ── Колбэки геймпада ─────────────────────────────────
    void onGamepad(usb_gamepad_cb_t cb)  { _gamepad_cb = cb; }
    void offGamepad()                    { _gamepad_cb = nullptr; }
    // 0=unknown (hex лог), 1=без стиков, 2=со стиками
    void setGamepadType(uint8_t type)    { _gamepad_type = type; }

    // ── Утилиты ──────────────────────────────────────────
    static char keycodeToChar(uint8_t keycode, uint8_t modifier);

    // ── Внутренние — вызываются из USB task ──────────────
    // (public чтобы статические C-колбэки библиотеки могли дёргать)
    void _pushKeyEvent(uint8_t keycode, uint8_t modifier, bool pressed);
    void _pushMouseEvent(int8_t dx, int8_t dy, uint8_t buttons, int8_t scroll);
    void _pushGamepadEvent(const GamepadState& state);
    void _setConnected(bool v)          { _connected = v; }
    uint8_t _getGamepadType() const     { return _gamepad_type; }

private:
    bool             _connected    = false;
    uint8_t          _gamepad_type = 0;
    usb_key_cb_t     _key_cb       = nullptr;
    usb_mouse_cb_t   _mouse_cb     = nullptr;
    usb_gamepad_cb_t _gamepad_cb   = nullptr;

    // ── Кольцевой буфер (lock-free SPSC) ─────────────────
    // USB task пишет (_w), main task читает (_r)
    volatile uint8_t _w = 0;
    volatile uint8_t _r = 0;
    UsbEvent         _buf[USB_EVENT_BUF_SIZE];

    void _push(const UsbEvent& e);
};

} // namespace pda2