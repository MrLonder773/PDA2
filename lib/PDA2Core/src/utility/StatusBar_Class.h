#pragma once
#include <lvgl.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════
//  StatusBar_Class
//  Прозрачный оверлей 320×27 на lv_layer_top() поверх экранов
//  приложений: время, Bluetooth, Wi-Fi, батарея.
//  Создаётся ДО QuickPanel — затемнение и шторки перекрывают бар.
//  Касания не перехватывает.
//  Время/BT/Wi-Fi читает сам в update(); батарею задаёт PDA.Power
//  через setBattery().
// ═══════════════════════════════════════════════════════════

class StatusBar_Class {
public:
    void begin();
    void update();                  // из PDA.update(), до Apps.tick()
    void toggle();                  // показать/скрыть бар
    
    // percent 0..100; < 0 — нет данных (число «--», заливка скрыта)
    void setBattery(int percent);

private:
    lv_obj_t* _bar         = nullptr;
    lv_obj_t* _lbl_time    = nullptr;
    lv_obj_t* _img_bt      = nullptr;
    lv_obj_t* _img_wifi    = nullptr;
    lv_obj_t* _fill        = nullptr;   // серая заливка батареи
    lv_obj_t* _img_battery = nullptr;   // контур
    lv_obj_t* _lbl_battery = nullptr;   // число заряда

    int     _battery   = -1;
    int16_t _time_key  = -2;            // hour*60+minute; -1 — нет времени; -2 — ещё не выводили
    uint32_t _time_ms  = 0;
    bool    _time_init = false;
    int8_t  _bt_vis    = -1;            // -1 — не применено
    int8_t  _wifi_lvl  = -2;            // -2 — не применено, -1 — скрыта, 0..4 — иконка

    void _update_time();
    void _apply_battery();
};