// ════════════════════════════════════════════════════════
//  PDA 2 SIM — Bt_Class_sim.cpp
//  Always-offline stub: на PC BLE-стека нет.
//  state() всегда BT_OFF, все операции — no-op / false.
//  BluetoothApp при BT_OFF показывает "недоступно" —
//  это ожидаемое и корректное поведение в симе.
// ════════════════════════════════════════════════════════

#include "Bt_Class.h"
#include "../pda2_log.h"

#ifdef PDA2_SIM

#define TAG "bt"

Bt_Class* Bt_Class::_instance = nullptr;

bool Bt_Class::begin() {
    if (_initialized) return true;
    _initialized = true;
    PDA_LOGI(TAG, "SIM: always offline (no BLE stack on PC)");
    return true;
}

void Bt_Class::update() {}

void Bt_Class::enable()  { PDA_LOGI(TAG, "SIM: enable() ignored, stays OFF"); }
void Bt_Class::disable() {}

void Bt_Class::startScan(uint8_t /*seconds*/) {}
void Bt_Class::stopScan() {}

bool Bt_Class::connect(uint8_t /*index*/) { return false; }
bool Bt_Class::connectByAddr(const char* /*mac_str*/, uint8_t /*addr_type*/) { return false; }
bool Bt_Class::connectByName(const char* /*name*/) { return false; }
void Bt_Class::disconnect() {}

const char* Bt_Class::connectedName() const { return ""; }

// ── Методы, доступные из file-local callback'ов в .cpp ───
// На sim колбэков никто не вызывает (нет BLE task), но
// определения нужны линкеру.
void Bt_Class::_onReport(const uint8_t* /*data*/, size_t /*len*/) {}
void Bt_Class::_addScanResult(const char* /*addr*/, uint8_t /*addr_type*/, const char* /*name*/) {}
void Bt_Class::_onConnect(const char* /*addr*/, const char* /*name*/) {}
void Bt_Class::_onDisconnect() {}
void Bt_Class::_connect_task(void* /*param*/) {}

#endif // PDA2_SIM