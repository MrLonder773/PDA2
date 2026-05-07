#pragma once
#include <PDA2.h>

class AccelApp : public PDA2App {
public:
    AccelApp() { name = "Accel"; }
    void onInit()                  override;
    void onOpen()                  override;
    void onTick(uint32_t delta_ms) override;

private:
    lv_obj_t* _canvas     = nullptr;
    void*     _canvas_buf = nullptr;

    lv_obj_t* _lbl_ax = nullptr, *_lbl_ay = nullptr, *_lbl_az = nullptr;
    lv_obj_t* _lbl_gx = nullptr, *_lbl_gy = nullptr, *_lbl_gz = nullptr;
    lv_obj_t* _lbl_no_imu = nullptr;

    float    _rot[3][3];   // матрица вращения (body frame, без gimbal lock)
    uint32_t _tick_acc = 0;

    void _redraw(const pda2_imu_t& d);
};