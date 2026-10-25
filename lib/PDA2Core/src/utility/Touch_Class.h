#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — Touch_Class.h
//  FT6236/FT6336U (чтение — через pda2_platform_touch_*).
//  Трансформация координат по rotation, жесты (home, свайпы вниз
//  из верхней зоны, свайп вверх из середины), отмена клика при
//  выходе пальца.
// ════════════════════════════════════════════════════════

#include <stdint.h>
#include <lvgl.h>
#include <functional>

// Флаг виджета: «сам обрабатывает перетаскивание» (карточка со свайпом и т.п.).
// Для таких виджетов отмена клика при выходе пальца не применяется.
// Слайдеры распознаются по типу, флаг им не нужен.
// ⚠️ LV_OBJ_FLAG_USER_1 не должен использоваться где-то ещё в проекте.
#define PDA2_FLAG_NO_CLICK_GUARD  LV_OBJ_FLAG_USER_1

class Touch_Class {
public:
    // Колбэки жестов — устанавливаются из PDA2.cpp при begin()
    std::function<void()> onHomeGesture     = nullptr;   // свайп вверх из нижней зоны
    std::function<void()> onSwipeUpMid      = nullptr;   // свайп вверх из середины экрана
    std::function<void()> onSwipeDownLeft   = nullptr;   // свайп вниз из верхней зоны, левая половина
    std::function<void()> onSwipeDownRight  = nullptr;   // то же, правая половина

    void begin(uint8_t rotation);
    void setRotation(uint8_t r);
    bool ok() const { return _ok; }

private:
    static constexpr int32_t  QP_SWIPE_MIN_DY      = 40;   // мин. сдвиг вниз для свайпа из верхней зоны, px
    static constexpr uint32_t SWIPE_UP_MAX_MS      = 500;  // свайпы вверх (home, середина): макс. длительность
    static constexpr int32_t  SWIPE_UP_TRAVEL_DIV  = 5;    // мин. путь вверх = высота экрана / 5 (20%)
    static constexpr int32_t  MID_SWIPE_MIN_PCT    = 30;   // свайп из середины: полоса старта, % высоты
    static constexpr int32_t  MID_SWIPE_MAX_PCT    = 70;

    uint8_t _rotation = 2;
    bool    _ok       = false;

    // LVGL indev
    static lv_indev_t* _indev;

    // Состояние жестов
    static int32_t  _swipe_start_y;
    static int32_t  _swipe_last_y;   // последняя позиция пальца
    static uint32_t _swipe_start_ms;
    static bool     _touching;
    static bool     _scroll_seen;    // за это касание LVGL начал скролл — свайп-середина не срабатывает

    static void _read_cb(lv_indev_t* indev, lv_indev_data_t* data);

    int16_t _qp_start_x = -1;
    int16_t _qp_start_y = -1;
    bool    _qp_fired   = false;
};