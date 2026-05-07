#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — Touch_Class.h
//  FT6236, raw I2C. Трансформация координат + swipe-home.
// ════════════════════════════════════════════════════════

#include <stdint.h>
#include <lvgl.h>
#include <functional>

class Touch_Class {
public:
    // onHomeGesture — устанавливается из PDA2.cpp при begin()
    std::function<void()> onHomeGesture = nullptr;

    void begin(uint8_t rotation);
    void setRotation(uint8_t r);
    bool ok() const { return _ok; }

private:
    uint8_t _rotation = 2;
    bool    _ok       = false;

    // LVGL indev
    static lv_indev_t* _indev;

    // Swipe-home состояние
    static int32_t  _swipe_start_y;
    static int32_t  _swipe_last_y;   // последняя позиция пальца
    static uint32_t _swipe_start_ms;
    static bool     _touching;

    static void _read_cb(lv_indev_t* indev, lv_indev_data_t* data);
};