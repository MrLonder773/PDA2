#pragma once
#include <PDA2.h>
#include <MadgwickAHRS.h>

class AccelApp : public PDA2App {
public:
    AccelApp() { name = "Accel"; }
    void onInit()                  override;
    void onOpen()                  override;
    void onTick(uint32_t delta_ms) override;

private:
    // ── Canvas ───────────────────────────────────────────────────────────
    lv_obj_t* _canvas     = nullptr;
    void*     _canvas_buf = nullptr;

    // ── Данные (нормальный режим) ─────────────────────────────────────────
    lv_obj_t* _data_panel = nullptr;
    lv_obj_t* _lbl_ax = nullptr, *_lbl_ay = nullptr, *_lbl_az = nullptr;
    lv_obj_t* _lbl_gx = nullptr, *_lbl_gy = nullptr, *_lbl_gz = nullptr;
    lv_obj_t* _lbl_no_imu = nullptr;

    // ── Калибровка (режим настройки осей) ────────────────────────────────
    lv_obj_t* _cal_panel      = nullptr;
    lv_obj_t* _src_btn[3][3]  = {};   // [ось][источник GX/GY/GZ]
    lv_obj_t* _sign_btn[3]    = {};
    lv_obj_t* _sign_lbl[3]    = {};
    bool      _cal_mode       = false;

    // ── Runtime ремаппинг осей ───────────────────────────────────────────
    int8_t _map[3];    // 0=gx, 1=gy, 2=gz
    int8_t _sign[3];   // +1 или -1

    // ── Madgwick + матрица ───────────────────────────────────────────────
    float    _rot[3][3];
    uint32_t _tick_acc = 0;
    Madgwick _filter;

    // ── Контексты колбэков ───────────────────────────────────────────────
    struct SrcCtx  { AccelApp* app; int axis; int src; };
    struct SignCtx { AccelApp* app; int axis; };
    SrcCtx  _src_ctx[3][3];
    SignCtx _sign_ctx[3];

    void _redraw(const pda2_imu_t& d);
    void _toggle_cal();
    void _update_cal_ui();
};