// ════════════════════════════════════════════════════════
//  PDA 2 LITE — Bt_Class.cpp
//  Стек: NimBLE-Arduino (h2zero @ ^2.0.0).   // ← было ^1.4.0
//  Архитектура: NimBLE GATT клиент.
//    Scan: NimBLEDevice::getScan() → onResult → queue
//    Connect: отдельный FreeRTOS task → getService(0x1812)
//             → registerForNotify(0x2A4D) → queue
//    Keys: _notify_cb → _onReport() → queue → update()
// ════════════════════════════════════════════════════════
#include "Bt_Class.h"
#include "../pda2_log.h"

#include <NimBLEDevice.h>

#define TAG "bt"

// ── Внутренние события очереди ───────────────────────────
enum BtEvtType : uint8_t {
    EVT_KEY        = 0,
    EVT_CONNECT    = 1,
    EVT_DISCONNECT = 2,
    EVT_SCAN_DEV   = 3,
    EVT_SCAN_DONE  = 4,
};

struct BtEvt {
    BtEvtType type;
    char      addr[18];
    char      name[32];
    uint8_t   addr_type;
    uint8_t   keycode;
    uint8_t   modifier;
    bool      pressed;
};

Bt_Class* Bt_Class::_instance = nullptr;

// ── Notify callback (BLE task → queue) ───────────────────
static void _notify_cb(NimBLERemoteCharacteristic* ch,
                       uint8_t* data, size_t len, bool isNotify) {
    if (Bt_Class::_instance)
        Bt_Class::_instance->_onReport(data, len);
}

// ── Scan device callbacks ─────────────────────────────────
class BtScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* adv) override {
        Bt_Class* self = Bt_Class::_instance;
        if (!self || !self->_queue) return;
        if (!adv->isAdvertisingService(NimBLEUUID("1812"))) return;

        BtEvt ev = {};
        ev.type      = EVT_SCAN_DEV;
        ev.addr_type = adv->getAddressType();
        strlcpy(ev.addr, adv->getAddress().toString().c_str(), sizeof(ev.addr));
        const char* n = adv->getName().c_str();
        strlcpy(ev.name, n[0] ? n : "HID Device", sizeof(ev.name));
        xQueueSend(self->_queue, &ev, 0);
    }

    void onScanEnd(const NimBLEScanResults& results, int reason) override {
        Bt_Class* self = Bt_Class::_instance;
        if (!self || !self->_queue) return;
        BtEvt ev = {}; ev.type = EVT_SCAN_DONE;
        xQueueSend(self->_queue, &ev, 0);
    }
};
static BtScanCallbacks s_scan_cbs;

// ── Client callbacks ──────────────────────────────────────
class BtClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* c) override {}
    void onDisconnect(NimBLEClient* c, int reason) override {
        Bt_Class* self = Bt_Class::_instance;
        if (!self || !self->_queue) return;
        BtEvt ev = {}; ev.type = EVT_DISCONNECT;
        xQueueSend(self->_queue, &ev, 0);
    }
};
static BtClientCallbacks s_client_cbs;

// ── begin ─────────────────────────────────────────────────
bool Bt_Class::begin() {
    if (_initialized) return true;   // ← guard
    _initialized = true;

    _instance = this;
    _queue = xQueueCreate(16, sizeof(BtEvt));
    if (!_queue) { PDA_LOGE(TAG, "queue create failed"); return false; }
    NimBLEDevice::init("");
    NimBLEDevice::setSecurityAuth(true, false, true);
    PDA_LOGI(TAG, "NimBLE init done");
    return true;
}

// ── enable / disable ─────────────────────────────────────
void Bt_Class::enable() {
    if (_state != BT_OFF) return;
    _state = BT_IDLE;
    PDA_LOGI(TAG, "enabled → IDLE");
}

void Bt_Class::disable() {
    if (_state == BT_OFF) return;
    if (_state == BT_SCANNING)  stopScan();
    if (_state == BT_CONNECTED) disconnect();
    _state = BT_OFF;
    PDA_LOGI(TAG, "disabled → OFF");
}

// ── startScan ────────────────────────────────────────────
void Bt_Class::startScan(uint8_t seconds) {
    if (_state != BT_IDLE) return;

    // Сохранить только подключённые устройства
    uint8_t n = 0;
    for (uint8_t i = 0; i < _dev_count; i++)
        if (_devices[i].connected) _devices[n++] = _devices[i];
    _dev_count = n;

    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->setScanCallbacks(&s_scan_cbs, false);
    scan->setActiveScan(true);
    scan->setInterval(100);
    scan->setWindow(99);
    scan->clearResults();
    scan->start((uint32_t)seconds * 1000, false);

    _state = BT_SCANNING;
    PDA_LOGI(TAG, "scan started (%ds)", seconds);
}

// ── stopScan ──────────────────────────────────────────────
void Bt_Class::stopScan() {
    if (_state != BT_SCANNING) return;

    // ← ФИКС: проверяем реальное состояние NimBLE,
    //   не только наш _state (он обновляется асинхронно через EVT_SCAN_DONE)
    if (NimBLEDevice::getScan()->isScanning()) {
        NimBLEDevice::getScan()->stop();
    }
}

// ── connect ───────────────────────────────────────────────
bool Bt_Class::connect(uint8_t index) {
    if (index >= _dev_count) return false;
    return connectByAddr(_devices[index].addr, _devices[index].addr_type);
}

bool Bt_Class::connectByAddr(const char* mac_str, uint8_t addr_type) {
    if (_state == BT_OFF || _connecting) return false;
    if (_state == BT_SCANNING)  stopScan();
    if (_state == BT_CONNECTED) disconnect();

    auto* p = new ConnectParams();
    strlcpy(p->addr, mac_str, sizeof(p->addr));
    p->addr_type = addr_type;
    p->name[0]   = '\0';
    for (uint8_t i = 0; i < _dev_count; i++) {
        if (strcmp(_devices[i].addr, mac_str) == 0) {
            strlcpy(p->name, _devices[i].name, sizeof(p->name));
            break;
        }
    }

    _connecting = true;
    xTaskCreate(_connect_task, "bt_conn", 4096, p, 5, nullptr);
    PDA_LOGI(TAG, "connect task → %s", mac_str);
    return true;
}

bool Bt_Class::connectByName(const char* name) {
    if (_state == BT_OFF || _connecting) return false;
    strlcpy(_auto_connect_name, name, sizeof(_auto_connect_name));
    _auto_connecting = true;
    startScan(10);
    PDA_LOGI(TAG, "auto-connect scan → name: %s", name);
    return true;
}

// ── disconnect ────────────────────────────────────────────
void Bt_Class::disconnect() {
    if (_state != BT_CONNECTED || !_client) return;
    ((NimBLEClient*)_client)->disconnect();
    // → BtClientCallbacks::onDisconnect → EVT_DISCONNECT → update() удалит client
}

const char* Bt_Class::connectedName() const {
    if (_conn_idx < 0 || _conn_idx >= (int8_t)_dev_count) return "";
    return _devices[_conn_idx].name;
}

// ── update (main task) ───────────────────────────────────
void Bt_Class::update() {
    BtEvt ev;
    while (xQueueReceive(_queue, &ev, 0) == pdTRUE) {
        switch (ev.type) {

            case EVT_KEY:
                if (_state == BT_CONNECTED && _key_cb)
                    _key_cb(ev.keycode, ev.modifier, ev.pressed);
                break;

            case EVT_CONNECT:
                // Гонка: disable() вызван пока connect task работал
                if (_state == BT_OFF) {
                    NimBLEDevice::deleteClient((NimBLEClient*)_client);
                    _client = nullptr;
                    _connecting = false;
                } else {
                    _onConnect(ev.addr, ev.name);
                }
                break;

            case EVT_DISCONNECT:
                if (_client) {
                    NimBLEDevice::deleteClient((NimBLEClient*)_client);
                    _client = nullptr;
                }
                _onDisconnect();
                break;

            case EVT_SCAN_DEV:
                _addScanResult(ev.addr, ev.addr_type, ev.name);
                break;

            case EVT_SCAN_DONE:
                if (_state == BT_SCANNING) _state = BT_IDLE;
                PDA_LOGI(TAG, "scan done, %u found", _dev_count);
                break;
        }
    }
}

// ── Handlers (main task) ─────────────────────────────────
void Bt_Class::_addScanResult(const char* addr, uint8_t addr_type, const char* name) {
    // Авто-коннект по имени
    if (_auto_connecting && strcmp(name, _auto_connect_name) == 0) {
        _auto_connecting = false;
        _auto_connect_name[0] = '\0';
        PDA_LOGI(TAG, "auto-connect match: %s [%s]", name, addr);
        connectByAddr(addr, addr_type);
        return;
    }

    // ← ПРОПАЛИ: проверка дублей и bounds check
    for (uint8_t i = 0; i < _dev_count; i++)
        if (strcmp(_devices[i].addr, addr) == 0) return;
    if (_dev_count >= BT_MAX_DEVICES) return;

    BtDevice& d = _devices[_dev_count++];
    strlcpy(d.addr, addr, sizeof(d.addr));
    d.addr_type = addr_type;
    strlcpy(d.name, name, sizeof(d.name));
    d.connected = false;
    PDA_LOGI(TAG, "found: %s [%s]", name, addr);
}

void Bt_Class::_onConnect(const char* addr, const char* name) {
    _conn_idx = -1;
    for (uint8_t i = 0; i < _dev_count; i++) {
        if (strcmp(_devices[i].addr, addr) == 0) {
            _devices[i].connected = true;
            _conn_idx = (int8_t)i;
            break;
        }
    }
    if (_conn_idx < 0 && _dev_count < BT_MAX_DEVICES) {
        BtDevice& d = _devices[_dev_count];
        strlcpy(d.addr, addr, sizeof(d.addr));
        strlcpy(d.name, name[0] ? name : "BLE HID", sizeof(d.name));
        d.connected = true;
        _conn_idx = (int8_t)_dev_count++;
    }
    _state = BT_CONNECTED;
    PDA_LOGI(TAG, "connected: %s", name);
}

void Bt_Class::_onDisconnect() {
    if (_conn_idx >= 0 && _conn_idx < (int8_t)_dev_count)
        _devices[_conn_idx].connected = false;
    _conn_idx   = -1;
    _connecting = false;
    _state      = BT_IDLE;
    memset(_last_report, 0, sizeof(_last_report));
    PDA_LOGI(TAG, "disconnected → IDLE");
}

// ── _onReport: HID report parser (BLE task context) ──────
// Boot-protocol keyboard: [mod, reserved, key0..key5]
// Те же keycodes что в Usb_Class — keycodeToChar() совместим.
void Bt_Class::_onReport(const uint8_t* data, size_t len) {
    if (!_queue || len != 8) return;  // только keyboard boot-protocol
    uint8_t mod = data[0];
    BtEvt ev    = {}; ev.type = EVT_KEY; ev.modifier = mod;

    // Отпущенные: были в _last_report, нет в data
    for (int i = 2; i < 8; i++) {
        if (!_last_report[i]) continue;
        bool still = false;
        for (size_t j = 2; j < len && j < 8; j++)
            if (data[j] == _last_report[i]) { still = true; break; }
        if (!still) {
            ev.keycode = _last_report[i]; ev.pressed = false;
            xQueueSend(_queue, &ev, 0);
        }
    }

    // Нажатые: есть в data, не было в _last_report
    for (size_t i = 2; i < len && i < 8; i++) {
        if (!data[i]) continue;
        bool was = false;
        for (int j = 2; j < 8; j++)
            if (_last_report[j] == data[i]) { was = true; break; }
        if (!was) {
            ev.keycode = data[i]; ev.pressed = true;
            xQueueSend(_queue, &ev, 0);
        }
    }

    size_t clen = len < 8 ? len : 8;
    memcpy(_last_report, data, clen);
    if (clen < 8) memset(_last_report + clen, 0, 8 - clen);
}

// ── Connect task ──────────────────────────────────────────
void Bt_Class::_connect_task(void* param) {
    auto* p    = (ConnectParams*)param;
    auto* self = _instance;

    if (!self || !self->_queue) {
        delete p; vTaskDelete(nullptr); return;
    }

    NimBLEClient* client = NimBLEDevice::createClient();
    client->setClientCallbacks(&s_client_cbs, false); // false = не удалять при disconnect

    PDA_LOGI(TAG, "connecting → %s (type %u)", p->addr, p->addr_type);
    bool ok = client->connect(NimBLEAddress(p->addr, p->addr_type));

    if (!ok) {
        PDA_LOGE(TAG, "connect() failed");
        NimBLEDevice::deleteClient(client);
        self->_connecting = false;
        BtEvt ev = {}; ev.type = EVT_DISCONNECT;
        xQueueSend(self->_queue, &ev, 0);
        delete p; vTaskDelete(nullptr); return;
    }

    // HID сервис
    NimBLERemoteService* svc = client->getService(NimBLEUUID("1812"));
    if (!svc) {
        PDA_LOGE(TAG, "HID service (0x1812) not found");
        self->_client     = client; // update() удалит после EVT_DISCONNECT
        self->_connecting = false;
        client->disconnect();       // → BtClientCallbacks::onDisconnect → EVT_DISCONNECT
        delete p; vTaskDelete(nullptr); return;
    }

    // Подписаться на Input Report (0x2A4D)
    int sub = 0;
    const auto& chars = svc->getCharacteristics(true);
    for (auto ch : chars) {
        if (ch->getUUID() == NimBLEUUID("2A4D") && ch->canNotify()) {
            if (ch->subscribe(true, _notify_cb)) sub++;
        }
    }
    PDA_LOGI(TAG, "subscribed to %d report char(s)", sub);

    self->_client     = client;
    self->_connecting = false;

    BtEvt ev = {}; ev.type = EVT_CONNECT; ev.addr_type = p->addr_type;
    strlcpy(ev.addr, p->addr, sizeof(ev.addr));
    strlcpy(ev.name, p->name[0] ? p->name : "BLE HID", sizeof(ev.name));
    xQueueSend(self->_queue, &ev, 0);

    delete p; vTaskDelete(nullptr);
}