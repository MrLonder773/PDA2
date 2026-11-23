#pragma once
#include <lvgl.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════
//  HomeBar_Class
//  Полоса home 114×5 на lv_layer_top() поверх всех экранов и шторок.
//  Создаётся ПОСЛЕ QuickPanel. Касания не перехватывает.
//  Идёт за пальцем по вертикали: drag()/release() из колбэков Touch
//  задают только ЦЕЛЬ, к которой полоса плавно движется в update()
//  (экспоненциальное сглаживание по времени, без lv_timer/lv_anim).
//  Предел хода задаёт Touch, полоса его не знает.
// ═══════════════════════════════════════════════════════════

class HomeBar_Class {
public:
    void begin();
    void update();           // из PDA.update(), после StatusBar.update()
    void drag(int32_t dy);   // цель: сдвиг вверх от исходного положения, px (>= 0)
    void release();          // цель: исходное положение (плавный возврат)

private:
    lv_obj_t* _bar      = nullptr;
    int32_t   _cur16    = 0;       // текущий сдвиг вверх, 1/16 px
    int32_t   _target16 = 0;       // целевой сдвиг вверх, 1/16 px
    int32_t   _applied  = 0;       // последний выставленный сдвиг, px
    bool      _dragging = false;   // true — палец на экране (быстрое следование)
    uint32_t  _last_ms  = 0;
};