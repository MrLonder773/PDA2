// ════════════════════════════════════════════════════════
//  PDA 2 — Touch_Class.cpp
//  FT6236/FT6336U (чтение — pda2_platform_touch_*), трансформация
//  по rotation, жесты.
//
//  Swipe-home: старт в нижних PDA2_HOME_SWIPE_PCT% экрана, быстрый
//  свайп вверх → onHomeGesture().
//  Swipe-mid: старт в полосе MID_SWIPE_MIN_PCT..MID_SWIPE_MAX_PCT
//  высоты, быстрый свайп вверх, LVGL не начал скролл → onSwipeUpMid().
//
//  Отмена клика: LVGL по умолчанию держит виджет нажатым, даже если
//  палец ушёл (LV_OBJ_FLAG_PRESS_LOCK) — и клик проходит при отпускании.
//  Поэтому при выходе пальца за hit-area виджета, на котором началось
//  касание, шлём ему PRESS_LOST и просим LVGL игнорировать палец до
//  отпускания: клика не будет, даже если палец вернулся (см. _click_guard).
//  Цель ищется в том же порядке слоёв, что и в LVGL: sys → top → экран.
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
bool        Touch_Class::_scroll_seen    = false;

// ── Глобальный указатель на экземпляр для callback ──────
static Touch_Class* _instance = nullptr;

// ── Отмена клика при выходе пальца за hit-area ───────────
// Порог = размер виджета (lv_obj_hit_test учитывает и расширенную
// зону нажатия — lv_obj_set_ext_click_area).
static lv_obj_t* _press_obj = nullptr;   // виджет, на котором началось касание

// Экран или системный слой (не виджет приложения)
static bool _is_root(lv_obj_t* obj) {
    return obj == lv_screen_active() || obj == lv_layer_top() || obj == lv_layer_sys();
}

// Виджет сам обрабатывает перетаскивание — отмена клика не применяется
static bool _handles_own_drag(lv_obj_t* obj) {
    return lv_obj_has_flag(obj, PDA2_FLAG_NO_CLICK_GUARD) ||
           lv_obj_check_type(obj, &lv_slider_class);
}

// Поиск цели в порядке слоёв LVGL: sys → top → активный экран
static lv_obj_t* _search_target(lv_point_t* pt) {
    lv_obj_t* obj = lv_indev_search_obj(lv_layer_sys(), pt);
    if (!obj) obj = lv_indev_search_obj(lv_layer_top(), pt);
    if (!obj) obj = lv_indev_search_obj(lv_screen_active(), pt);
    return obj;
}

// true, если у виджета есть прокручиваемый предок с содержимым за границей:
// там выход пальца — это начало скролла, им управляет LVGL.
static bool _in_scrollable(lv_obj_t* obj) {
    for (lv_obj_t* p = lv_obj_get_parent(obj); p; p = lv_obj_get_parent(p)) {
        if (!lv_obj_has_flag(p, LV_OBJ_FLAG_SCROLLABLE)) continue;
        if (lv_obj_get_scroll_top(p)    > 0 || lv_obj_get_scroll_bottom(p) > 0 ||
            lv_obj_get_scroll_left(p)   > 0 || lv_obj_get_scroll_right(p)  > 0)
            return true;
    }
    return false;
}

// Вызывается на каждом кадре нажатия.
static void _click_guard(lv_indev_t* indev, bool first_frame, lv_point_t pt) {
    if (first_frame) {
        _press_obj = nullptr;

        lv_obj_t* obj = _search_target(&pt);
        // Цель — ближайший кликабельный виджет (палец мог попасть в label внутри кнопки)
        while (obj && !_is_root(obj) && !lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE))
            obj = lv_obj_get_parent(obj);
        if (!obj || _is_root(obj)) return;
        if (_handles_own_drag(obj)) return;
        if (_in_scrollable(obj)) return;

        _press_obj = obj;
        return;
    }

    if (!_press_obj) return;

    // Скролл начался — это не клик, дальше решает LVGL
    if (lv_indev_get_scroll_obj(indev)) { _press_obj = nullptr; return; }
    // Экран/виджет могли удалиться посреди касания
    if (!lv_obj_is_valid(_press_obj))   { _press_obj = nullptr; return; }

    if (!lv_obj_hit_test(_press_obj, &pt)) {
        lv_obj_t* obj = _press_obj;
        _press_obj = nullptr;                                 // один раз за касание
        lv_obj_send_event(obj, LV_EVENT_PRESS_LOST, indev);   // снять состояние :pressed
        lv_indev_wait_release(indev);                         // до отпускания палец игнорируется,
                                                              // при отпускании клика не будет
    }
}

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
        _transform(raw_x, raw_y, tx, ty, (_instance->_rotation + PDA2_DISPLAY_ROT_OFFSET) & 3);
#else
        // На PC мышь уже в конечном логическом пространстве окна —
        // корректировать физическую ориентацию сенсора нечего.
        tx = raw_x;
        ty = raw_y;
#endif

        data->point.x = tx;
        data->point.y = ty;
        data->state   = LV_INDEV_STATE_PRESSED;

        const bool first_frame = !_touching;
        if (first_frame) {
            // Начало касания
            _touching       = true;
            _scroll_seen    = false;
            _swipe_start_y  = ty;
            _swipe_start_ms = pda2_platform_now_ms();
            // ── QP: запомнить старт ──────────────────────
            _instance->_qp_start_x = (int16_t)tx;
            _instance->_qp_start_y = (int16_t)ty;
            _instance->_qp_fired   = false;
        }
        // Обновляем последнюю позицию каждый фрейм
        _swipe_last_y = ty;

        // ── Отмена клика (жесты ниже считают по тем же tx/ty) ──
        lv_point_t pt = {tx, ty};
        _click_guard(indev, first_frame, pt);

        // ── Скролл LVGL начался — свайп-середина не должен срабатывать ──
        if (!first_frame && lv_indev_get_scroll_obj(indev)) _scroll_seen = true;

        // ── QP: детект свайпа вниз из верхней зоны ──────
        if (!_instance->_qp_fired && _instance->_qp_start_y >= 0) {
            const bool landscape = (_instance->_rotation == 1 || _instance->_rotation == 3);
            const int32_t eff_w = landscape ? PDA2_SCREEN_H : PDA2_SCREEN_W;
            const int32_t eff_h = landscape ? PDA2_SCREEN_W : PDA2_SCREEN_H;
            if (_instance->_qp_start_y < (eff_h * PDA2_QP_SWIPE_TOP_PCT / 100)) {
                int32_t dy = ty - _instance->_qp_start_y;  // > 0 = вниз
                if (dy > QP_SWIPE_MIN_DY) {
                    _instance->_qp_fired = true;
                    if (_instance->_qp_start_x < eff_w / 2) {
                        if (_instance->onSwipeDownLeft)  _instance->onSwipeDownLeft();
                    } else {
                        if (_instance->onSwipeDownRight) _instance->onSwipeDownRight();
                    }
                }
            }
        }
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        _press_obj  = nullptr;

        if (_touching && _swipe_start_y >= 0) {
            int32_t screen_h = (_instance->_rotation == 1 || _instance->_rotation == 3)
                               ? PDA2_SCREEN_W
                               : PDA2_SCREEN_H;
            uint32_t elapsed  = pda2_platform_now_ms() - _swipe_start_ms;
            int32_t  delta_y  = _swipe_start_y - _swipe_last_y;  // > 0 = движение вверх

            // Свайпы вверх: палец ушёл вверх минимум на 1/SWIPE_UP_TRAVEL_DIV
            // высоты экрана (20%) за < SWIPE_UP_MAX_MS.
            bool moved_up     = delta_y > (screen_h / SWIPE_UP_TRAVEL_DIV);
            bool fast_enough  = elapsed < SWIPE_UP_MAX_MS;
            // home: старт в нижних PDA2_HOME_SWIPE_PCT% экрана
            bool started_low  = _swipe_start_y > (screen_h * PDA2_HOME_SWIPE_PCT / 100);
            // mid: старт в полосе MID_SWIPE_MIN_PCT..MID_SWIPE_MAX_PCT высоты
            bool started_mid  = _swipe_start_y >= (screen_h * MID_SWIPE_MIN_PCT / 100) &&
                                _swipe_start_y <= (screen_h * MID_SWIPE_MAX_PCT / 100);

            if (started_low && moved_up && fast_enough) {
                if (_instance && _instance->onHomeGesture) {
                    _instance->onHomeGesture();
                }
            } else if (started_mid && moved_up && fast_enough && !_scroll_seen) {
                if (_instance && _instance->onSwipeUpMid) {
                    _instance->onSwipeUpMid();
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