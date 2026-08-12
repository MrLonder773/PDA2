#include "AccelApp.h"
#include <math.h>
#include <string.h>
#include <esp_heap_caps.h>

static const char* TAG = "accel";

static constexpr int16_t CANVAS_W = PDA2_ACCEL_CANVAS_W;
static constexpr int16_t CANVAS_H = PDA2_ACCEL_CANVAS_H;
static constexpr int16_t STRIP_H  = PDA2_ACCEL_STRIP_H;

// ── Куб ──────────────────────────────────────────────────────────────────
static const float VERTS[8][3] = {
    {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
    {-1,-1, 1},{1,-1, 1},{1,1, 1},{-1,1, 1}
};
static const uint8_t EDGES[12][2] = {
    {0,1},{1,2},{2,3},{3,0},
    {4,5},{5,6},{6,7},{7,4},
    {0,4},{1,5},{2,6},{3,7}
};

// ── Draw helpers ─────────────────────────────────────────────────────────

static void rotate3d(const float rot[3][3],
                     float ix, float iy, float iz,
                     float& ox, float& oy, float& oz) {
    ox = rot[0][0]*ix + rot[0][1]*iy + rot[0][2]*iz;
    oy = rot[1][0]*ix + rot[1][1]*iy + rot[1][2]*iz;
    oz = rot[2][0]*ix + rot[2][1]*iy + rot[2][2]*iz;
}

static void proj(float x, float y, float z,
                 int16_t& px, int16_t& py) {
    float s = (float)PDA2_ACCEL_FOCAL /
              ((float)PDA2_ACCEL_FOCAL + z * PDA2_ACCEL_CUBE_SIZE + 200.0f);
    px = (int16_t)(PDA2_ACCEL_CUBE_CX + x * PDA2_ACCEL_CUBE_SIZE * s);
    py = (int16_t)(PDA2_ACCEL_CUBE_CY + y * PDA2_ACCEL_CUBE_SIZE * s);
}

static void draw_line(lv_layer_t* lay,
                      int16_t x1, int16_t y1,
                      int16_t x2, int16_t y2,
                      lv_color_t col, int32_t w = 2) {
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.p1.x = (lv_value_precise_t)x1;  d.p1.y = (lv_value_precise_t)y1;
    d.p2.x = (lv_value_precise_t)x2;  d.p2.y = (lv_value_precise_t)y2;
    d.color = col;  d.width = w;  d.opa = LV_OPA_COVER;
    lv_draw_line(lay, &d);
}

static void draw_dot(lv_layer_t* lay, int16_t x, int16_t y) {
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = lv_color_white();
    d.bg_opa   = LV_OPA_COVER;
    d.radius   = LV_RADIUS_CIRCLE;
    lv_area_t a = { (int32_t)x-3, (int32_t)y-3,
                    (int32_t)x+2, (int32_t)y+2 };
    lv_draw_rect(lay, &d, &a);
}

static void draw_axis_label(lv_layer_t* lay,
                             int16_t x, int16_t y,
                             const char* text, lv_color_t col) {
    lv_draw_label_dsc_t d;
    lv_draw_label_dsc_init(&d);
    d.color = col;
    d.font  = &lv_font_montserrat_12;
    d.opa   = LV_OPA_COVER;
    d.text  = text;
    lv_area_t a = { (int32_t)x, (int32_t)y,
                    (int32_t)(x+14), (int32_t)(y+14) };
    lv_draw_label(lay, &d, &a);
}

// ── onInit ───────────────────────────────────────────────────────────────

void AccelApp::onInit() {
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(PDA2_ACCEL_BG_COLOR), 0);
    lv_obj_set_style_bg_opa  (screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all (screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);

    // Canvas (PSRAM) — 280×370, тап переключает режим
    size_t buf_sz = (size_t)CANVAS_W * CANVAS_H * 2;
    _canvas_buf = heap_caps_malloc(buf_sz, MALLOC_CAP_SPIRAM);
    if (!_canvas_buf) { PDA_LOGE(TAG, "PSRAM alloc failed"); return; }
    _canvas = lv_canvas_create(screen);
    lv_canvas_set_buffer(_canvas, _canvas_buf, CANVAS_W, CANVAS_H,
                         LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(_canvas, PDA2_ACCEL_CANVAS_X, PDA2_ACCEL_CANVAS_Y);
    lv_canvas_fill_bg(_canvas, lv_color_hex(PDA2_ACCEL_BG_COLOR), LV_OPA_COVER);
    lv_obj_add_flag(_canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(_canvas, [](lv_event_t* e) {
        ((AccelApp*)lv_event_get_user_data(e))->_toggle_cal();
    }, LV_EVENT_CLICKED, this);

    // NO IMU
    _lbl_no_imu = lv_label_create(screen);
    lv_label_set_text(_lbl_no_imu, "NO IMU");
    lv_obj_set_style_text_font (_lbl_no_imu, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(_lbl_no_imu, lv_color_hex(0xef4444), 0);
    lv_obj_set_style_bg_opa   (_lbl_no_imu, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color (_lbl_no_imu, lv_color_hex(PDA2_ACCEL_BG_COLOR), 0);
    lv_obj_align(_lbl_no_imu, LV_ALIGN_CENTER, 0, -(STRIP_H / 2));
    lv_obj_add_flag(_lbl_no_imu, LV_OBJ_FLAG_HIDDEN);

    // Стрип — 100px снизу
    lv_obj_t* strip = lv_obj_create(screen);
    lv_obj_set_size(strip, PDA2_SCREEN_W, STRIP_H);
    lv_obj_set_pos (strip, 0, PDA2_SCREEN_H - STRIP_H);
    lv_obj_set_style_bg_color   (strip, lv_color_hex(PDA2_ACCEL_BG_COLOR), 0);
    lv_obj_set_style_bg_opa     (strip, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(strip, lv_color_hex(0x1f2937), 0);
    lv_obj_set_style_border_width(strip, 1, 0);
    lv_obj_set_style_border_side (strip, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_pad_all    (strip, 0, 0);
    lv_obj_set_scrollbar_mode   (strip, LV_SCROLLBAR_MODE_OFF);

    // ── Панель данных ────────────────────────────────────────────────────
    _data_panel = lv_obj_create(strip);
    lv_obj_set_size(_data_panel, PDA2_SCREEN_W, STRIP_H);
    lv_obj_set_pos (_data_panel, 0, 0);
    lv_obj_set_style_bg_opa     (_data_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_data_panel, 0, 0);
    lv_obj_set_style_pad_all    (_data_panel, 0, 0);
    lv_obj_set_scrollbar_mode   (_data_panel, LV_SCROLLBAR_MODE_OFF);

    auto make_pair = [&](const char* name, int16_t x, int16_t y,
                         lv_color_t col) -> lv_obj_t* {
        lv_obj_t* nl = lv_label_create(_data_panel);
        lv_label_set_text(nl, name);
        lv_obj_set_style_text_font (nl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(nl, lv_color_hex(0x4b5563), 0);
        lv_obj_set_style_bg_opa    (nl, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color  (nl, lv_color_hex(PDA2_ACCEL_BG_COLOR), 0);
        lv_obj_set_pos(nl, x, y);
        lv_obj_t* vl = lv_label_create(_data_panel);
        lv_label_set_text(vl, "+0.00");
        lv_obj_set_style_text_font (vl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(vl, col, 0);
        lv_obj_set_style_bg_opa    (vl, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color  (vl, lv_color_hex(PDA2_ACCEL_BG_COLOR), 0);
        lv_obj_set_pos(vl, x + 24, y);
        return vl;
    };

    static constexpr int16_t PX  = 14;
    static constexpr int16_t PX2 = PDA2_SCREEN_W / 2 + 8;
    static constexpr int16_t RH  = 30;
    static constexpr int16_t PY  = 8;
    lv_color_t cx = lv_color_hex(0x60a5fa);
    lv_color_t cy = lv_color_hex(0x34d399);
    lv_color_t cz = lv_color_hex(0xfbbf24);
    _lbl_ax = make_pair("AX", PX,  PY,          cx);
    _lbl_gx = make_pair("GX", PX2, PY,          cx);
    _lbl_ay = make_pair("AY", PX,  PY + RH,     cy);
    _lbl_gy = make_pair("GY", PX2, PY + RH,     cy);
    _lbl_az = make_pair("AZ", PX,  PY + RH * 2, cz);
    _lbl_gz = make_pair("GZ", PX2, PY + RH * 2, cz);

    // ── Панель калибровки ─────────────────────────────────────────────────
    // Тап по canvas → показать; тап снова → скрыть.
    // Для каждой визуальной оси (X/Y/Z): выбрать физический источник GX/GY/GZ
    // и знак +1/-1. Активная кнопка подсвечена цветом оси.
    _cal_panel = lv_obj_create(strip);
    lv_obj_set_size(_cal_panel, PDA2_SCREEN_W, STRIP_H);
    lv_obj_set_pos (_cal_panel, 0, 0);
    lv_obj_set_style_bg_color   (_cal_panel, lv_color_hex(PDA2_ACCEL_BG_COLOR), 0);
    lv_obj_set_style_bg_opa     (_cal_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_cal_panel, 0, 0);
    lv_obj_set_style_pad_all    (_cal_panel, 0, 0);
    lv_obj_set_scrollbar_mode   (_cal_panel, LV_SCROLLBAR_MODE_OFF);

    static constexpr int16_t BTN_W  = 48;
    static constexpr int16_t BTN_H  = 22;
    static constexpr int16_t BTN_G  = 4;
    static constexpr int16_t SRC_X  = 38;
    static constexpr int16_t SIGN_X = 210;
    static constexpr int16_t SIGN_W = 50;

    const char* ax_names[]  = {"X", "Y", "Z"};
    const char* src_names[] = {"GX", "GY", "GZ"};

    for (int ax = 0; ax < 3; ax++) {
        int16_t row_cy = PY + ax * RH + RH / 2;

        // Подпись оси
        lv_obj_t* al = lv_label_create(_cal_panel);
        lv_label_set_text(al, ax_names[ax]);
        lv_obj_set_style_text_font (al, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(al, lv_color_hex(0x9ca3af), 0);
        lv_obj_set_style_bg_opa   (al, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color (al, lv_color_hex(PDA2_ACCEL_BG_COLOR), 0);
        lv_obj_set_pos(al, 14, row_cy - 8);

        // Кнопки источника GX / GY / GZ
        for (int src = 0; src < 3; src++) {
            _src_ctx[ax][src] = {this, ax, src};

            lv_obj_t* btn = lv_obj_create(_cal_panel);
            lv_obj_set_size(btn, BTN_W, BTN_H);
            lv_obj_set_pos (btn, SRC_X + src * (BTN_W + BTN_G),
                                 row_cy - BTN_H / 2);
            lv_obj_set_style_bg_color   (btn, lv_color_hex(0x374151), 0);
            lv_obj_set_style_bg_opa     (btn, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(btn, 0, 0);
            lv_obj_set_style_pad_all    (btn, 0, 0);
            lv_obj_set_style_radius     (btn, 4, 0);
            lv_obj_set_scrollbar_mode   (btn, LV_SCROLLBAR_MODE_OFF);
            lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                auto* ctx = (SrcCtx*)lv_event_get_user_data(e);
                ctx->app->_map[ctx->axis] = ctx->src;
                ctx->app->_update_cal_ui();
            }, LV_EVENT_CLICKED, &_src_ctx[ax][src]);

            lv_obj_t* lbl = lv_label_create(btn);
            lv_label_set_text(lbl, src_names[src]);
            lv_obj_set_style_text_font (lbl, &lv_font_montserrat_12, 0);
            lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
            lv_obj_set_style_bg_opa    (lbl, LV_OPA_TRANSP, 0);
            lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);

            _src_btn[ax][src] = btn;
        }

        // Кнопка знака (+1 / -1)
        _sign_ctx[ax] = {this, ax};

        lv_obj_t* sbtn = lv_obj_create(_cal_panel);
        lv_obj_set_size(sbtn, SIGN_W, BTN_H);
        lv_obj_set_pos (sbtn, SIGN_X, row_cy - BTN_H / 2);
        lv_obj_set_style_bg_color   (sbtn, lv_color_hex(0x22c55e), 0);
        lv_obj_set_style_bg_opa     (sbtn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(sbtn, 0, 0);
        lv_obj_set_style_pad_all    (sbtn, 0, 0);
        lv_obj_set_style_radius     (sbtn, 4, 0);
        lv_obj_set_scrollbar_mode   (sbtn, LV_SCROLLBAR_MODE_OFF);
        lv_obj_add_flag(sbtn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(sbtn, [](lv_event_t* e) {
            auto* ctx = (SignCtx*)lv_event_get_user_data(e);
            ctx->app->_sign[ctx->axis] *= -1;
            ctx->app->_update_cal_ui();
        }, LV_EVENT_CLICKED, &_sign_ctx[ax]);

        lv_obj_t* slbl = lv_label_create(sbtn);
        lv_label_set_text(slbl, "+1");
        lv_obj_set_style_text_font (slbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(slbl, lv_color_white(), 0);
        lv_obj_set_style_bg_opa    (slbl, LV_OPA_TRANSP, 0);
        lv_obj_align(slbl, LV_ALIGN_CENTER, 0, 0);

        _sign_btn[ax] = sbtn;
        _sign_lbl[ax] = slbl;
    }

    lv_obj_add_flag(_cal_panel, LV_OBJ_FLAG_HIDDEN);
}

// ── onOpen ───────────────────────────────────────────────────────────────

void AccelApp::onOpen() {
    memset(_rot, 0, sizeof(_rot));
    _rot[0][0] = _rot[1][1] = _rot[2][2] = 1.0f;

    _filter.begin(1000.0f / PDA2_ACCEL_TICK_MS);
    _tick_acc = PDA2_ACCEL_TICK_MS;

    // Загружаем дефолты из конфига
    _map[0]  = PDA2_ACCEL_GYRO_MAP_X;
    _map[1]  = PDA2_ACCEL_GYRO_MAP_Y;
    _map[2]  = PDA2_ACCEL_GYRO_MAP_Z;
    _sign[0] = PDA2_ACCEL_GYRO_SIGN_X;
    _sign[1] = PDA2_ACCEL_GYRO_SIGN_Y;
    _sign[2] = PDA2_ACCEL_GYRO_SIGN_Z;
    _update_cal_ui();

    // Сбрасываем в нормальный режим при повторном открытии
    if (_cal_mode) _toggle_cal();

    if (!PDA.Imu.ok()) {
        lv_obj_clear_flag(_lbl_no_imu, LV_OBJ_FLAG_HIDDEN);
        PDA_LOGW(TAG, "IMU not available");
    } else {
        lv_obj_add_flag(_lbl_no_imu, LV_OBJ_FLAG_HIDDEN);
    }
}

// ── onTick ───────────────────────────────────────────────────────────────

void AccelApp::onTick(uint32_t delta_ms) {
    if (!PDA.Imu.ok()) return;

    pda2_imu_t d = PDA.Imu.get();

    // Madgwick фьюзит gyro (°/s) + accel (g) → нет дрейфа pitch/roll
    _filter.updateIMU(d.gx, d.gy, d.gz, d.ax, d.ay, d.az);

    // Euler → матрица вращения (ZYX конвенция)
    float roll  = _filter.getRoll()  * DEG_TO_RAD;
    float pitch = _filter.getPitch() * DEG_TO_RAD;
    float yaw   = _filter.getYaw()   * DEG_TO_RAD;
    float cr = cosf(roll),  sr = sinf(roll);
    float cp = cosf(pitch), sp = sinf(pitch);
    float cy = cosf(yaw),   sy = sinf(yaw);

    _rot[0][0] =  cy*cp;
    _rot[0][1] =  cy*sp*sr - sy*cr;
    _rot[0][2] =  cy*sp*cr + sy*sr;
    _rot[1][0] =  sy*cp;
    _rot[1][1] =  sy*sp*sr + cy*cr;
    _rot[1][2] =  sy*sp*cr - cy*sr;
    _rot[2][0] = -sp;
    _rot[2][1] =  cp*sr;
    _rot[2][2] =  cp*cr;

    // Ремаппинг столбцов матрицы: визуальная ось j берётся из физической _map[j] со знаком _sign[j]
    float remapped[3][3];
    for (int row = 0; row < 3; row++) {
        remapped[row][0] = _sign[0] * _rot[row][_map[0]];
        remapped[row][1] = _sign[1] * _rot[row][_map[1]];
        remapped[row][2] = _sign[2] * _rot[row][_map[2]];
    }
    memcpy(_rot, remapped, sizeof(_rot));

    _tick_acc += delta_ms;
    if (_tick_acc < PDA2_ACCEL_TICK_MS) return;
    _tick_acc = 0;
    _redraw(d);
}

// ── _toggle_cal ──────────────────────────────────────────────────────────

void AccelApp::_toggle_cal() {
    _cal_mode = !_cal_mode;
    if (_cal_mode) {
        lv_obj_add_flag  (_data_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(_cal_panel,  LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(_data_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag  (_cal_panel,  LV_OBJ_FLAG_HIDDEN);
    }
}

// ── _update_cal_ui ───────────────────────────────────────────────────────

void AccelApp::_update_cal_ui() {
    static const uint32_t AXIS_COLORS[3] = {0x60a5fa, 0x34d399, 0xfbbf24};

    for (int ax = 0; ax < 3; ax++) {
        for (int src = 0; src < 3; src++) {
            lv_color_t col = (_map[ax] == src)
                ? lv_color_hex(AXIS_COLORS[ax])
                : lv_color_hex(0x374151);
            lv_obj_set_style_bg_color(_src_btn[ax][src], col, 0);
        }
        bool pos = (_sign[ax] > 0);
        lv_obj_set_style_bg_color(_sign_btn[ax],
            pos ? lv_color_hex(0x22c55e) : lv_color_hex(0xef4444), 0);
        lv_label_set_text(_sign_lbl[ax], pos ? "+1" : "-1");
    }
}

// ── _redraw ──────────────────────────────────────────────────────────────

void AccelApp::_redraw(const pda2_imu_t& d) {
    if (!_canvas) return;

    lv_canvas_fill_bg(_canvas, lv_color_hex(PDA2_ACCEL_BG_COLOR), LV_OPA_COVER);

    lv_layer_t layer;
    lv_canvas_init_layer(_canvas, &layer);

    // Куб
    int16_t px[8], py[8];
    for (int i = 0; i < 8; i++) {
        float rx, ry, rz;
        rotate3d(_rot, VERTS[i][0], VERTS[i][1], VERTS[i][2], rx, ry, rz);
        proj(rx, ry, rz, px[i], py[i]);
    }
    for (auto& e : EDGES)
        draw_line(&layer, px[e[0]], py[e[0]], px[e[1]], py[e[1]],
                  lv_color_hex(0xe5e7eb));
    for (int i = 0; i < 8; i++)
        draw_dot(&layer, px[i], py[i]);

    // Оси X/Y/Z
    const float AU = (float)PDA2_ACCEL_AXIS_LEN / PDA2_ACCEL_CUBE_SIZE;
    struct { float x, y, z; uint32_t c; const char* lbl; } axes[] = {
        { AU,  0,  0, 0xef4444, "X" },
        {  0, AU,  0, 0x22c55e, "Y" },
        {  0,  0, AU, 0x3b82f6, "Z" },
    };
    for (auto& a : axes) {
        float rx, ry, rz;
        rotate3d(_rot, a.x, a.y, a.z, rx, ry, rz);
        int16_t apx, apy;
        proj(rx, ry, rz, apx, apy);
        draw_line(&layer, PDA2_ACCEL_CUBE_CX, PDA2_ACCEL_CUBE_CY,
                  apx, apy, lv_color_hex(a.c), 2);
        draw_axis_label(&layer, apx + 4, apy - 8, a.lbl, lv_color_hex(a.c));
    }

    lv_canvas_finish_layer(_canvas, &layer);

    // Обновляем данные только в нормальном режиме
    if (!_cal_mode) {
        char buf[12];
        snprintf(buf, sizeof(buf), "%+.2f g", d.ax); lv_label_set_text(_lbl_ax, buf);
        snprintf(buf, sizeof(buf), "%+.2f g", d.ay); lv_label_set_text(_lbl_ay, buf);
        snprintf(buf, sizeof(buf), "%+.2f g", d.az); lv_label_set_text(_lbl_az, buf);
        snprintf(buf, sizeof(buf), "%+5.1f",  d.gx); lv_label_set_text(_lbl_gx, buf);
        snprintf(buf, sizeof(buf), "%+5.1f",  d.gy); lv_label_set_text(_lbl_gy, buf);
        snprintf(buf, sizeof(buf), "%+5.1f",  d.gz); lv_label_set_text(_lbl_gz, buf);
    }
}