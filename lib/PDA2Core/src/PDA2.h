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

#include <Arduino.h>
#include <Wire.h>
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