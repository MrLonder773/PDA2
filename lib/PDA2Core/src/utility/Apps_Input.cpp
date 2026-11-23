// ════════════════════════════════════════════════════════
//  PDA 2 — Apps_Input.cpp
//  Клавиши (USB + BLE HID) и навигация по launcher.
// ════════════════════════════════════════════════════════

#include <PDA2.h>
#include "Apps_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"

// ── next / prev / openSelected ────────────────────────────

void Apps_Class::next() {
    if (_count == 0) return;
    _select((_selected_id + 1) % _count);
}

void Apps_Class::prev() {
    if (_count == 0) return;
    _select((_selected_id - 1 + _count) % _count);
}

void Apps_Class::openSelected() {
    if (_selected_id >= 0 && _selected_id < _count)
        open((uint8_t)_selected_id);
}

// ── Клавиатурный колбэк (USB + BLE) ──────────────────────
void Apps_Class::_key_cb(uint8_t keycode, uint8_t modifier, bool pressed) {
    if (!pressed || !_instance) return;

    if (_instance->isOpen()) {
        if (keycode == 41) {
            _instance->close();
        } else {
            PDA2App* app = _instance->current();
            if (app) app->onKey(keycode, modifier);
        }
    } else {
        switch (keycode) {
            case 80: case 82: _instance->prev();         break;
            case 79: case 81: _instance->next();         break;
            case 40:          _instance->openSelected(); break;
        }
    }
}