// ════════════════════════════════════════════════════════
//  PDA 2 LITE — Bt_Class.h
//  BLE HID Host. Стек: NimBLE-Arduino (h2zero @ ^1.4.0).
//  NimBLE callbacks → FreeRTOS queue → update() (main task).
//  NimBLE заголовки — только в .cpp, не здесь.
// ════════════════════════════════════════════════════════
#pragma once

#include <stdint.h>
#include <string.h>

#ifndef PDA2_SIM
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#endif

#define BT_MAX_DEVICES 8

enum BtState : uint8_t {
    BT_OFF = 0,
    BT_IDLE,
    BT_SCANNING,
    BT_CONNECTED,
};

struct BtDevice {
    char    addr[18];     // "aa:bb:cc:dd:ee:ff" (NimBLE lowercase)
    uint8_t addr_type;    // 0=public, 1=random
    char    name[32];
    bool    connected;
};

typedef void (*bt_key_cb_t)(uint8_t keycode, uint8_t modifier, bool pressed);

class Bt_Class {
public:
    bool begin();
    void update();

    void enable();
    void disable();

    void startScan(uint8_t seconds = 10);
    void stopScan();

    bool connect(uint8_t index);
    bool connectByAddr(const char* mac_str, uint8_t addr_type = 1);
    bool connectByName(const char* name);  // скан + авто-коннект по имени
    void disconnect();

    bool _initialized = false;

    BtState          state()           const { return _state; }
    uint8_t          deviceCount()     const { return _dev_count; }
    const BtDevice&  device(uint8_t i) const { return _devices[i]; }
    const char*      connectedName()   const;

    void onKey(bt_key_cb_t cb) { _key_cb = cb; }
    void offKey()              { _key_cb = nullptr; }

    // ── Доступно из file-local callback'ов в .cpp ─────────
    void          _onReport(const uint8_t* data, size_t len);
    void*         _queue      = nullptr;   // QueueHandle_t на esp32 (== void*), не используется на sim
    bool          _connecting = false;
    void*         _client     = nullptr;  // NimBLEClient*, удаляется в update()

    static Bt_Class* _instance;
    
private:
    BtState       _state     = BT_OFF;
    BtDevice      _devices[BT_MAX_DEVICES] = {};
    uint8_t       _dev_count = 0;
    int8_t        _conn_idx  = -1;
    bt_key_cb_t   _key_cb    = nullptr;
    uint8_t       _last_report[8] = {};

    struct ConnectParams {
        char    addr[18];
        char    name[32];
        uint8_t addr_type;
    };

    void _addScanResult(const char* addr, uint8_t addr_type, const char* name);
    void _onConnect(const char* addr, const char* name);
    void _onDisconnect();

    static void _connect_task(void* param);
    bool _auto_connecting = false;
    char _auto_connect_name[32] = {};
};