// ════════════════════════════════════════════════════════
//  PDA 2 — PDA2.cpp
//  Реализация PDA2Class.
//  Глобальный экземпляр PDA живёт здесь.
// ════════════════════════════════════════════════════════

#include "PDA2.h"
#include <esp_heap_caps.h>

// ── Глобальный экземпляр ─────────────────────────────────
pda2::PDA2Class PDA;

// ── begin ─────────────────────────────────────────────────
void pda2::PDA2Class::begin(config_t cfg) {
    Serial.begin(115200);
    delay(200);
    PDA_LOGI("core", "PDA 2 starting... " PDA2_VERSION);

    // I2C
    Wire.begin(PDA2_PIN_SDA, PDA2_PIN_SCL);
    Wire.setClock(100000);
    PDA_LOGI("core", "I2C ok (SDA=%d SCL=%d 100kHz)", PDA2_PIN_SDA, PDA2_PIN_SCL);
    
    // Prefs
    Prefs.begin();

    if (cfg.load_prefs) {
        cfg.brightness = Prefs.getBrightness();
        cfg.rotation   = Prefs.getRotation();
    }

    // Подсистемы — строго в этом порядке
    Fs.begin();
    if (!Fs.internal.ok()) PDA_LOGW("core", "Fs internal not ready");
    if (!Fs.sdAvailable()) PDA_LOGI("core", "Fs SD not available");

    if (!Display.begin(cfg.brightness, cfg.rotation)) {
        PDA_LOGE("core", "Display init failed — halting");
        while(true) delay(1000);
    }

    Rtc.begin();
    if (!Rtc.ok()) PDA_LOGW("core", "Rtc not found");
    else PDA_LOGI("core", "Rtc ok");
    Imu.begin();
    if (!Imu.ok()) PDA_LOGW("core", "Imu not found");
    else PDA_LOGI("core", "Imu ok");
    
    QuickPanel.begin();
    Touch.begin(cfg.rotation);
    Touch.onHomeGesture    = []() { PDA.Apps.close(); };
    Touch.onSwipeDownLeft  = []() { PDA.QuickPanel.openNotif(); };
    Touch.onSwipeDownRight = []() { PDA.QuickPanel.openSettings(); };

    Usb.begin();
    if (!Usb.ok()) PDA_LOGI("core", "Usb: no device");
    Usb.onKey(Apps_Class::_key_cb);            // ← USB колбэк

    Bt.begin();
    Bt.onKey(Apps_Class::_key_cb);             // ← BLE колбэк, тот же _key_cb

    if (Prefs.getBtEnabled()) {
        _bt_restore_pending = true;             // ← BT-STACK-1: enable() на первом тике
        PDA_LOGI("core", "Bt restore pending");
    }

    _last_tick_ms = millis();

    PDA_LOGI("core", "Heap: %u  PSRAM: %u", freeHeap(), freePsram());
    PDA_LOGI("core", "PDA.begin() done");
}

// ── notify ────────────────────────────────────────────────
void pda2::PDA2Class::notify(const char* app_name, const char* text) {
    QuickPanel.pushNotif(app_name, text);
}

// ── update ────────────────────────────────────────────────
void pda2::PDA2Class::update() {
    // BT-STACK-1: NimBLE не готов сразу после begin() — откладываем на первый тик
    if (_bt_restore_pending) {
        _bt_restore_pending = false;
        Bt.enable();
        String name = Prefs.getBtLastDeviceName();
        if (name.length() > 0) {
            PDA_LOGI("core", "Bt auto-reconnect → %s", name.c_str());
            Bt.connectByName(name.c_str());
        }
    }

    uint32_t now      = millis();
    uint32_t delta_ms = now - _last_tick_ms;
    _last_tick_ms     = now;

    Apps.tick(delta_ms);
    Usb.update();
    Bt.update();
}

// ── toast ─────────────────────────────────────────────────
void pda2::PDA2Class::toast(const char* text, uint32_t ms) {
    Apps.showToast(text, ms);
}

// ── утилиты ───────────────────────────────────────────────
uint32_t pda2::PDA2Class::freeHeap() {
    return (uint32_t)heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
}

uint32_t pda2::PDA2Class::freePsram() {
    return (uint32_t)heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
}