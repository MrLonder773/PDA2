// ════════════════════════════════════════════════════════
//  PDA 2 — Touch_Class.cpp
//  FT6236 raw I2C, трансформация по rotation, swipe-home.
//
//  Swipe-home: касание в нижней четверти экрана,
//  быстрый свайп вверх (<500ms) → onHomeGesture()
// ════════════════════════════════════════════════════════

#include "Touch_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"
#include "../pda2_platform.h"

// ── Статические члены ────────────────────────────────────
lv_indev_t* Touch_Class::_indev          = nullptr;
int32_t     Touch_Class::_swipe_start_y  = -1;
int32_t     Touch_Class::_swipe_last_y   = -1;
uint32_t    Touch_Class::_swipe_start_ms = 0;
bool        Touch_Class::_touching       = false;

// ── Глобальный указатель на экземпляр для callback ──────
static Touch_Class* _instance = nullptr;

// ── Трансформация координат по rotation ──────────────────
static void _transform(int32_t raw_x, int32_t raw_y,
                       int32_t& tx, int32_t& ty,
                       uint8_t rotation)
{
    const int32_t W = PDA2_SCREEN_W;
    const int32_t H = PDA2_SCREEN_H;
    switch (rotation) {
        case 0:  tx = raw_x;       ty = raw_y;        break;
        case 1:  tx = raw_y;       ty = W-1 - raw_x;  break;
        case 2:  tx = W-1 - raw_x; ty = H-1 - raw_y;  break;
        case 3:  tx = H-1 - raw_y; ty = raw_x;        break;
        default: tx = raw_x;       ty = raw_y;         break;
    }
}

// ── LVGL indev read callback ─────────────────────────────
void Touch_Class::_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    int32_t raw_x, raw_y;
    bool pressed = pda2_platform_touch_read(raw_x, raw_y);

    if (pressed) {
        int32_t tx, ty;
#ifndef PDA2_SIM
        _transform(raw_x, raw_y, tx, ty, _instance->_rotation);
#else
        // На PC мышь уже в конечном логическом пространстве окна —
        // корректировать физическую ориентацию сенсора нечего.
        tx = raw_x;
        ty = raw_y;
#endif

        data->point.x = tx;
        data->point.y = ty;
        data->state   = LV_INDEV_STATE_PRESSED;

        if (!_touching) {
            // Начало касания
            _touching       = true;
            _swipe_start_y  = ty;
            _swipe_start_ms = pda2_platform_now_ms();
            // ── QP: запомнить старт ──────────────────────
            _instance->_qp_start_x = (int16_t)tx;
            _instance->_qp_start_y = (int16_t)ty;
            _instance->_qp_fired   = false;
        }
        // Обновляем последнюю позицию каждый фрейм
        _swipe_last_y = ty;

        // ── QP: детект свайпа вниз из верхней зоны ──────
        if (!_instance->_qp_fired && _instance->_qp_start_y >= 0) {
            int32_t eff_h = (_instance->_rotation == 1 || _instance->_rotation == 3)
                            ? PDA2_SCREEN_W : PDA2_SCREEN_H;
            if (_instance->_qp_start_y < (eff_h * PDA2_QP_SWIPE_TOP_PCT / 100)) {
                int32_t dy = ty - _instance->_qp_start_y;  // > 0 = вниз
                if (dy > 40) {
                    _instance->_qp_fired = true;
                    if (_instance->_qp_start_x < PDA2_SCREEN_W / 2) {
                        if (_instance->onSwipeDownLeft)  _instance->onSwipeDownLeft();
                    } else {
                        if (_instance->onSwipeDownRight) _instance->onSwipeDownRight();
                    }
                }
            }
        }
    } else {
        data->state = LV_INDEV_STATE_RELEASED;

        if (_touching && _swipe_start_y >= 0) {
            int32_t screen_h = (_instance->_rotation == 1 || _instance->_rotation == 3)
                               ? PDA2_SCREEN_W
                               : PDA2_SCREEN_H;
            uint32_t elapsed  = pda2_platform_now_ms() - _swipe_start_ms;
            int32_t  delta_y  = _swipe_start_y - _swipe_last_y;  // > 0 = движение вверх

            // Условия свайп-home:
            //   1. начало в нижних PDA2_HOME_SWIPE_PCT% экрана
            //   2. палец ушёл вверх минимум на 20% высоты экрана
            //   3. за < 500мс
            bool started_low  = _swipe_start_y > (screen_h * PDA2_HOME_SWIPE_PCT / 100);
            bool moved_up     = delta_y > (screen_h / 5);
            bool fast_enough  = elapsed < 500;

            if (started_low && moved_up && fast_enough) {
                if (_instance && _instance->onHomeGesture) {
                    _instance->onHomeGesture();
                }
            }
            _swipe_start_y = -1;
            _swipe_last_y  = -1;
            // ── QP: сброс ────────────────────────────────
            _instance->_qp_start_x = -1;
            _instance->_qp_start_y = -1;
        }
        _touching = false;
    }
}

// ── Реализация Touch_Class ───────────────────────────────
void Touch_Class::begin(uint8_t rotation) {
    _instance = this;
    _rotation = rotation;

    if (!pda2_platform_touch_begin()) {
        PDA_LOGE("touch", "touch device not found");
        _ok = false;
        return;
    }

    // Создать LVGL indev
    _indev = lv_indev_create();
    lv_indev_set_type(_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(_indev, _read_cb);

    _ok = true;
    PDA_LOGI("touch", "touch ok, LVGL indev created");
}

void Touch_Class::setRotation(uint8_t r) {
    _rotation = r;
}