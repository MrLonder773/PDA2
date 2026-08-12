// ════════════════════════════════════════════════════════
//  PDA 2 — BluetoothApp
//  4 состояния: OFF / IDLE / SCANNING / CONNECTED
//  Навигация: onKey (d-pad / BLE клавиатура)
// ════════════════════════════════════════════════════════
#pragma once
#include <PDA2.h>

class BluetoothApp : public PDA2App {
public:
    BluetoothApp() { name = "BT"; }

    void onInit()                                  override;
    void onOpen()                                  override;
    void onClose()                                 override;
    void onTick(uint32_t)                          override;
    void onKey(uint8_t keycode, uint8_t modifier)  override;

private:
    lv_obj_t* _status_lbl  = nullptr;
    lv_obj_t* _scan_lbl    = nullptr;
    lv_obj_t* _action_btn  = nullptr;
    lv_obj_t* _action_lbl  = nullptr;
    lv_obj_t* _disable_btn = nullptr;   // [Выкл BT], hidden когда BT_OFF
    lv_obj_t* _disable_lbl = nullptr;

    static const uint8_t ROWS_MAX = BT_MAX_DEVICES;
    lv_obj_t* _dev_rows[ROWS_MAX] = {};
    lv_obj_t* _dev_lbls[ROWS_MAX] = {};

    static void _btn_cb(lv_event_t* e);

    uint8_t  _focus       = 0;   // 0=action, 1=disable, 2..N+1=dev[N-1]
    uint8_t  _focus_max   = 0;
    BtState  _last_state  = (BtState)0xFF;
    uint8_t  _last_devcnt = 0xFF;
    
    void _doAction();
    void _rebuildUi();
    void _updateFocus();
    void _updateStatus();
};