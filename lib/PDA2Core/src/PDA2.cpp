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
    PDA_LOGI("core", "PDA 2 starting... v2.0.6");

    // I2C
    Wire.begin(PDA2_PIN_SDA, PDA2_PIN_SCL);
    Wire.setClock(100000);
    PDA_LOGI("core", "I2C ok (SDA=%d SCL=%d 100kHz)", PDA2_PIN_SDA, PDA2_PIN_SCL);

    // Prefs
    Prefs.begin();

    // Загрузить настройки из NVS если запрошено
    if (cfg.load_prefs) {
        cfg.brightness = Prefs.getBrightness();
        cfg.rotation   = Prefs.getRotation();
    }

    // Подсистемы — строго в этом порядке
    Fs.begin();
    if (!Fs.ok())      PDA_LOGW("core", "Fs not ready");

    if (!Display.begin(cfg.brightness, cfg.rotation)) {
        PDA_LOGE("core", "Display init failed — halting");
        while(true) delay(1000);
    }
    // Display не имеет ok() — ошибка alloc логируется внутри   // [A-06]

    Touch.begin(cfg.rotation);
    if (!Touch.ok())   PDA_LOGW("core", "Touch not ready");

    Rtc.begin();
    if (!Rtc.ok())     PDA_LOGW("core", "Rtc not ready");

    Imu.begin();
    if (!Imu.ok())     PDA_LOGW("core", "Imu not ready");

    // Swipe-home → Apps.close()
    Touch.onHomeGesture = []() {
        PDA.Apps.close();
    };

    _last_tick_ms = millis();

    PDA_LOGI("core", "Heap: %u  PSRAM: %u", freeHeap(), freePsram());
    PDA_LOGI("core", "PDA.begin() done");
}

// ── update ────────────────────────────────────────────────
void pda2::PDA2Class::update() {
    uint32_t now      = millis();
    uint32_t delta_ms = now - _last_tick_ms;
    _last_tick_ms     = now;

    Apps.tick(delta_ms);   // → onTick / onBackground / lv_timer_handler
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