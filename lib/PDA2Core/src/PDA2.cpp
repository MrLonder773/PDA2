// ════════════════════════════════════════════════════════
//  PDA 2 — PDA2.cpp
//  Реализация PDA2Class.
//  Глобальный экземпляр PDA живёт здесь.
// ════════════════════════════════════════════════════════

#include "PDA2.h"
#include "pda2_platform.h"
#include "pda2_log.h"

// ── Глобальный экземпляр ─────────────────────────────────
pda2::PDA2Class PDA;

// ── begin ─────────────────────────────────────────────────
void pda2::PDA2Class::begin(config_t cfg) {
    pda2_platform_begin();
    PDA_LOGI("core", "PDA 2 starting... " PDA2_VERSION);

    // I2C
    pda2_platform_i2c_begin(PDA2_PIN_SDA, PDA2_PIN_SCL);
    PDA_LOGI("core", "I2C ok (SDA=%d SCL=%d 100kHz)", PDA2_PIN_SDA, PDA2_PIN_SCL);
    Memory.log("boot");

    // Prefs
    Prefs.begin();

    if (cfg.load_prefs) {
        cfg.brightness = Prefs.getBrightness();
        cfg.rotation   = Prefs.getRotation();
    }

    // Подсистемы — строго в этом порядке
    Fs.begin(Prefs);
    if (!Fs.internal.ok()) PDA_LOGW("core", "Fs internal not ready");
    if (!Fs.sdAvailable()) PDA_LOGI("core", "Fs SD not available");
    Memory.log("after Fs");

    if (!Display.begin(cfg.brightness, cfg.rotation)) {
        PDA_LOGE("core", "Display init failed — halting");
        while(true) pda2_platform_sleep_ms(1000);
    }
    Memory.log("after Display");

    Rtc.begin();
    if (!Rtc.ok()) PDA_LOGW("core", "Rtc not found");
    else PDA_LOGI("core", "Rtc ok");
    Memory.log("after Rtc");
    Imu.begin();
    if (!Imu.ok()) PDA_LOGW("core", "Imu not found");
    else PDA_LOGI("core", "Imu ok%s", Imu.magOk() ? " (+mag)" : "");
    Memory.log("after Imu");
    Bme.begin();
    if (Bme.ok()) PDA_LOGI("core", "Bme ok");
    Memory.log("after Bme");
    
    StatusBar.begin();
    QuickPanel.begin();
    HomeBar.begin();
    Touch.begin(cfg.rotation);
    // Home и свайп вверх из середины: если открыта шторка — закрывают её, иначе home закрывает приложение
    Touch.onHomeGesture    = []() { if (PDA.QuickPanel.isOpen()) PDA.QuickPanel.close(); else PDA.Apps.close(); };
    Touch.onSwipeUpMid     = []() { if (PDA.QuickPanel.isOpen()) PDA.QuickPanel.close(); };
    Touch.onSwipeDownLeft  = []() { PDA.QuickPanel.openNotif(); };
    Touch.onSwipeDownRight = []() { PDA.QuickPanel.openSettings(); };
    Touch.onDoubleTapTop = []() { if (!PDA.QuickPanel.isOpen()) PDA.StatusBar.toggle(); };
    Touch.onHomeDrag    = [](int32_t dy) { PDA.HomeBar.drag(dy); };
    Touch.onHomeDragEnd = []() { PDA.HomeBar.release(); };
    Memory.log("after StatusBar+QuickPanel+HomeBar+Touch");

    Usb.begin();
    if (!Usb.ok()) PDA_LOGI("core", "Usb: no device");
    Usb.onKey(Apps_Class::_key_cb);            // ← USB колбэк
    Memory.log("after Usb");

    Bt.begin();
    Bt.onKey(Apps_Class::_key_cb);             // ← BLE колбэк, тот же _key_cb
    Memory.log("after Bt");

    Wifi.begin();                      // автоподключение, если Prefs.getWifi()
    Memory.log("after Wifi");
    if (Prefs.getBtEnabled()) {
        _bt_restore_pending = true;             // ← BT-STACK-1: enable() на первом тике
        PDA_LOGI("core", "Bt restore pending");
    }

    _last_tick_ms = pda2_platform_now_ms();

    {
        const MemSnapshot m = Memory.get();
        if (m.valid) PDA_LOGI("core", "Heap: %u  PSRAM: %u", (unsigned)m.internal.free, (unsigned)m.psram.free);
    }
    Memory.logTasks("begin done");
    PDA_LOGI("core", "PDA.begin() done");
}

// ── notify ────────────────────────────────────────────────
void pda2::PDA2Class::notify(const char* app_name, const char* text) {
    QuickPanel.pushNotif(app_name, text);
}

// ── update ────────────────────────────────────────────────
void pda2::PDA2Class::update() {
    // BT-STACK-1: NimBLE поднимается в Bt.enable(); восстановление — на первом тике, после begin()
    if (_bt_restore_pending) {
        _bt_restore_pending = false;
        Bt.enable();
        Memory.log("bt enable");
        Memory.logTasks("bt enable");
        PdaStr32 name = Prefs.getBtLastDeviceName();
        if (name.length() > 0) {
            PDA_LOGI("core", "Bt auto-reconnect → %s", name.c_str());
            Bt.connectByName(name.c_str());
        }
    }

    uint32_t now      = pda2_platform_now_ms();
    uint32_t delta_ms = now - _last_tick_ms;
    _last_tick_ms     = now;

    // Imu: датчики отдают данные на 100 Гц, а опрос стоит ~2.5 мс на I2C 100 кГц —
    // чаще раза в 10 мс шину не грузим. До Apps.tick(), чтобы onTick() видел свежий кэш.
    static uint32_t imu_last_ms = 0;
    if (now - imu_last_ms >= 10) {
        imu_last_ms = now;
        Imu.update();
    }
    Bme.update(); // сам троттлит до 1 Гц
    StatusBar.update();
    HomeBar.update();
    Apps.tick(delta_ms);
    Usb.update();
    Bt.update();
    Wifi.update();

#ifdef PDA2_MEMLOG
    // периодический срез: видно утечки и фрагментацию во времени
    static uint32_t mem_last_ms = 0;
    if (now - mem_last_ms >= 10000) {
        mem_last_ms = now;
        Memory.log("tick 10s");
        static bool tasks_logged = false;
        if (!tasks_logged) { tasks_logged = true; Memory.logTasks("tick 10s"); }
    }
#endif
}

// ── toast ─────────────────────────────────────────────────
void pda2::PDA2Class::toast(const char* text, uint32_t ms) {
    Apps.showToast(text, ms);
}