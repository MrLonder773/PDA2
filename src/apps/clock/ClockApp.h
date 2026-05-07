#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — ClockApp.h
//  Часы. Показывает время HH:MM:SS и дату.
// ════════════════════════════════════════════════════════

#include <PDA2.h>

class ClockApp : public PDA2App {
public:
    ClockApp() { name = "Clock"; }

    void onInit()  override;
    void onOpen()  override;
    void onClose() override;
    void onTick(uint32_t delta_ms) override;

private:
    lv_obj_t* _lbl_time = nullptr;   // HH:MM:SS
    lv_obj_t* _lbl_date = nullptr;   // Monday, 27.04.2026

    uint8_t  _prev_hh    = 0xFF;
    uint8_t  _prev_mm    = 0xFF;
    uint8_t  _prev_ss    = 0xFF;
    uint8_t  _prev_day   = 0xFF;   // [A-75]
    uint8_t  _prev_month = 0xFF;   // [A-75]
    uint16_t _prev_year  = 0xFFFF; // [A-75]
    
    uint32_t _tick_acc = 0;

    void _update_time(const pda2_time_t& t);
    void _update_date(const pda2_time_t& t);

    static const char* _weekday_str(uint8_t wd);
};