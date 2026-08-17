/*
 * PDA2
 * Copyright (C) 2026 MrLonder773
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — PDA2.h
//  ЕДИНСТВЕННЫЙ include во всём проекте.
//  Паттерн: M5Unified. Один глобальный объект PDA,
//  подсистемы — value members, прямой доступ.
//
//  В любом файле проекта:
//    #include <PDA2.h>
//    PDA.Rtc.get() / PDA.Apps.open(0) / PDA.toast("ok")
// ════════════════════════════════════════════════════════

#include "pda2_config.h"
#include "pda2_log.h"
#include "PDA2App.h"

#include "utility/Display_Class.h"
#include "utility/Fs_Class.h"
#include "utility/Prefs_Class.h"
#include "utility/Apps_Class.h"
#include "utility/QuickPanel_Class.h"
#include "utility/Font_Class.h"
#include "utility/Usb_Class.h"
#include "utility/Bt_Class.h"       // ← добавлено
#include "utility/Touch_Class.h"
#include "utility/Rtc_Class.h"
#include "utility/Imu_Class.h"

#include <lvgl.h>

namespace pda2 {

struct config_t {
    uint8_t  brightness = 200;
    uint8_t  rotation   = PDA2_ROTATION;
    bool     load_prefs = true;
};

class PDA2Class {
public:
    Display_Class    Display;
    Fs_Class         Fs;
    Prefs_Class      Prefs;
    Apps_Class       Apps;
    QuickPanel_Class QuickPanel;
    Font_Class       Fonts;
    Usb_Class        Usb;
    Bt_Class         Bt;            // ← добавлено
    Touch_Class      Touch;
    Rtc_Class        Rtc;
    Imu_Class        Imu;

    void     begin(config_t cfg = config_t());
    void     update();
    config_t config() const { return config_t{}; }

    void     toast(const char* text, uint32_t ms = 2000);
    uint32_t freeHeap();
    uint32_t freePsram();
    void     notify(const char* app_name, const char* text);

private:
    uint32_t _last_tick_ms       = 0;
    bool     _bt_restore_pending = false;  // ← добавлено
};

} // namespace pda2

extern pda2::PDA2Class PDA;