#include "AccelApp.h"
#include <math.h>
#include <string.h>
#include <esp_heap_caps.h>

static const char* TAG = "accel";

// Высота canvas: экран − статус-бар (28) − полоса данных (110)
static constexpr int16_t CANVAS_H = PDA2_SCREEN_H - 28 - 110;

// Вершины куба в нормализованном пространстве (±1)
static const float VERTS[8][3] = {
    {-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
    {-1,-1, 1}, {1,-1, 1}, {1,1, 1}, {-1,1, 1}
};
static const uint8_t EDGES[12][2] = {
    {0,1},{1,2},{2,3},{3,0},
    {4,5},{5,6},{6,7},{7,4},
    {0,4},{1,5},{2,6},{3,7}
};

// ── Вспомогательные функции ──────────────────────────────────────────────

static void rotate3d(const float rot[3][3],
                     float ix, float iy, float iz,
                     float& ox, float& oy, float& oz) {
    ox = rot[0][0]*ix + rot[0][1]*iy + rot[0][2]*iz;
    oy = rot[1][0]*ix + rot[1][1]*iy + rot[1][2]*iz;
    oz = rot[2][0]*ix + rot[2][1]*iy + rot[2][2]*iz;
}

static void rot_integrate(float rot[3][3], float ax, float ay, float az) {
    const float dr[3][3] = {
        { 1.0f,  -az,   ay },
        {   az, 1.0f,  -ax },
        {  -ay,   ax, 1.0f }
    };
    float tmp[3][3] = {};
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            for (int k = 0; k < 3; k++)
                tmp[i][j] += rot[i][k] * dr[k][j];
    memcpy(rot, tmp, sizeof(tmp));
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
    d.p1.x = (lv_value_precise_t)x1;
    d.p1.y = (lv_value_precise_t)y1;
    d.p2.x = (lv_value_precise_t)x2;
    d.p2.y = (lv_value_precise_t)y2;
    d.color = col;
    d.width = w;
    d.opa   = LV_OPA_COVER;
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

// ── onInit ───────────────────────────────────────────────────────────────

void AccelApp::onInit() {
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(PDA2_ACCEL_BG_COLOR), 0);
    lv_obj_set_style_bg_opa (screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);

    // Статус-бар
    lv_obj_t* bar = lv_obj_create(screen);
    lv_obj_set_size(bar, PDA2_SCREEN_W, 28);
    lv_obj_align(bar, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(PDA2_ACCEL_BG_COLOR), 0);
    lv_obj_set_style_bg_opa (bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_t* bar_lbl = lv_label_create(bar);
    lv_label_set_text(bar_lbl, "PDA 2");
    lv_obj_set_style_text_font(bar_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(bar_lbl, lv_color_hex(0xaaaaaa), 0);
    lv_obj_align(bar_lbl, LV_ALIGN_LEFT_MID, 14, 0);

    // Canvas (PSRAM)
    size_t buf_sz = (size_t)PDA2_SCREEN_W * CANVAS_H * 2;
    _canvas_buf = heap_caps_malloc(buf_sz, MALLOC_CAP_SPIRAM);
    if (!_canvas_buf) {
        PDA_LOGE(TAG, "PSRAM alloc failed (%u bytes)", (unsigned)buf_sz);
        return;
    }
    _canvas = lv_canvas_create(screen);
    lv_canvas_set_buffer(_canvas, _canvas_buf,
                         PDA2_SCREEN_W, CANVAS_H, LV_COLOR_FORMAT_RGB565);
    lv_obj_align(_canvas, LV_ALIGN_TOP_LEFT, 0, 28);
    lv_canvas_fill_bg(_canvas, lv_color_hex(PDA2_ACCEL_BG_COLOR), LV_OPA_COVER);

    // "NO IMU"
    _lbl_no_imu = lv_label_create(screen);
    lv_label_set_text(_lbl_no_imu, "NO IMU");
    lv_obj_set_style_text_font(_lbl_no_imu, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(_lbl_no_imu, lv_color_hex(0xef4444), 0);
    lv_obj_set_style_bg_opa(_lbl_no_imu, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(_lbl_no_imu, lv_color_hex(PDA2_ACCEL_BG_COLOR), 0);
    lv_obj_align(_lbl_no_imu, LV_ALIGN_CENTER, 0, -60);
    lv_obj_add_flag(_lbl_no_imu, LV_OBJ_FLAG_HIDDEN);

    // Полоса данных
    lv_obj_t* strip = lv_obj_create(screen);
    lv_obj_set_size(strip, PDA2_SCREEN_W, 110);
    lv_obj_align(strip, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(strip, lv_color_hex(PDA2_ACCEL_BG_COLOR), 0);
    lv_obj_set_style_bg_opa (strip, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(strip, lv_color_hex(0x1f2937), 0);
    lv_obj_set_style_border_width(strip, 1, 0);
    lv_obj_set_style_border_side (strip, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_pad_all(strip, 0, 0);

    auto make_pair = [&](const char* axis_name,
                         int16_t x, int16_t y,
                         lv_color_t col) -> lv_obj_t* {
        lv_obj_t* nl = lv_label_create(strip);
        lv_label_set_text(nl, axis_name);
        lv_obj_set_style_text_font(nl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(nl, lv_color_hex(0x4b5563), 0);
        lv_obj_set_style_bg_opa(nl, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(nl, lv_color_hex(PDA2_ACCEL_BG_COLOR), 0);
        lv_obj_set_pos(nl, x, y);

        lv_obj_t* vl = lv_label_create(strip);
        lv_label_set_text(vl, "+0.00");
        lv_obj_set_style_text_font(vl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(vl, col, 0);
        lv_obj_set_style_bg_opa(vl, LV_OPA_COVER, 0);   // TEAR-1
        lv_obj_set_style_bg_color(vl, lv_color_hex(PDA2_ACCEL_BG_COLOR), 0);
        lv_obj_set_pos(vl, x + 24, y);
        return vl;
    };

    static constexpr int16_t PX  = 14;
    static constexpr int16_t PX2 = PDA2_SCREEN_W / 2 + 8;
    static constexpr int16_t RH  = 28;
    static constexpr int16_t PY  = 12;
    lv_color_t cx = lv_color_hex(0x60a5fa);
    lv_color_t cy = lv_color_hex(0x34d399);
    lv_color_t cz = lv_color_hex(0xfbbf24);

    _lbl_ax = make_pair("AX", PX,  PY,          cx);
    _lbl_gx = make_pair("GX", PX2, PY,          cx);
    _lbl_ay = make_pair("AY", PX,  PY + RH,     cy);
    _lbl_gy = make_pair("GY", PX2, PY + RH,     cy);
    _lbl_az = make_pair("AZ", PX,  PY + RH * 2, cz);
    _lbl_gz = make_pair("GZ", PX2, PY + RH * 2, cz);
}

// ── onOpen ───────────────────────────────────────────────────────────────

void AccelApp::onOpen() {
    const float rx = 0.4f, ry = 0.6f;
    const float cx = cosf(rx), sx = sinf(rx);
    const float cy = cosf(ry), sy = sinf(ry);
    _rot[0][0] =  cy;    _rot[0][1] = sy*sx;  _rot[0][2] = sy*cx;
    _rot[1][0] =  0.0f;  _rot[1][1] = cx;     _rot[1][2] = -sx;
    _rot[2][0] = -sy;    _rot[2][1] = cy*sx;  _rot[2][2] = cy*cx;

    _tick_acc = PDA2_ACCEL_TICK_MS;

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
    float dt = delta_ms * 0.001f;

    // Ремаппинг физических осей BMI160 → визуальные оси куба.
    // Настраивается под конкретное расположение IMU на плате через pda2_config.h:
    //   GYRO_MAP_X/Y/Z — какую физическую ось (0=gx,1=gy,2=gz) брать
    //   GYRO_SIGN_X/Y/Z — +1 или -1 (инвертировать если куб крутится не туда)
    const float raw[3] = { d.gx, d.gy, d.gz };
    float w[3] = {
        (float)PDA2_ACCEL_GYRO_SIGN_X * raw[PDA2_ACCEL_GYRO_MAP_X],
        (float)PDA2_ACCEL_GYRO_SIGN_Y * raw[PDA2_ACCEL_GYRO_MAP_Y],
        (float)PDA2_ACCEL_GYRO_SIGN_Z * raw[PDA2_ACCEL_GYRO_MAP_Z],
    };

    // Мёртвая зона — убирает drift от gyro bias в покое
    if (fabsf(w[0]) < PDA2_ACCEL_GYRO_DEADZONE) w[0] = 0.0f;
    if (fabsf(w[1]) < PDA2_ACCEL_GYRO_DEADZONE) w[1] = 0.0f;
    if (fabsf(w[2]) < PDA2_ACCEL_GYRO_DEADZONE) w[2] = 0.0f;

    rot_integrate(_rot,
                  w[0] * dt * DEG_TO_RAD,
                  w[1] * dt * DEG_TO_RAD,
                  w[2] * dt * DEG_TO_RAD);

    _tick_acc += delta_ms;
    if (_tick_acc < PDA2_ACCEL_TICK_MS) return;
    _tick_acc = 0;
    _redraw(d);
}

// ── _redraw ──────────────────────────────────────────────────────────────

void AccelApp::_redraw(const pda2_imu_t& d) {
    if (!_canvas) return;

    lv_canvas_fill_bg(_canvas, lv_color_hex(PDA2_ACCEL_BG_COLOR), LV_OPA_COVER);

    lv_layer_t layer;
    lv_canvas_init_layer(_canvas, &layer);

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

    const float AU = (float)PDA2_ACCEL_AXIS_LEN / PDA2_ACCEL_CUBE_SIZE;
    struct { float x, y, z; uint32_t c; } axes[] = {
        { AU,  0,  0, 0xef4444 },
        {  0, AU,  0, 0x22c55e },
        {  0,  0, AU, 0x3b82f6 },
    };
    for (auto& a : axes) {
        float rx, ry, rz;
        rotate3d(_rot, a.x, a.y, a.z, rx, ry, rz);
        int16_t apx, apy;
        proj(rx, ry, rz, apx, apy);
        draw_line(&layer, PDA2_ACCEL_CUBE_CX, PDA2_ACCEL_CUBE_CY,
                  apx, apy, lv_color_hex(a.c));
    }

    lv_canvas_finish_layer(_canvas, &layer);

    char buf[12];
    snprintf(buf, sizeof(buf), "%+.2f g", d.ax); lv_label_set_text(_lbl_ax, buf);
    snprintf(buf, sizeof(buf), "%+.2f g", d.ay); lv_label_set_text(_lbl_ay, buf);
    snprintf(buf, sizeof(buf), "%+.2f g", d.az); lv_label_set_text(_lbl_az, buf);
    snprintf(buf, sizeof(buf), "%+5.1f",  d.gx); lv_label_set_text(_lbl_gx, buf);
    snprintf(buf, sizeof(buf), "%+5.1f",  d.gy); lv_label_set_text(_lbl_gy, buf);
    snprintf(buf, sizeof(buf), "%+5.1f",  d.gz); lv_label_set_text(_lbl_gz, buf);
}