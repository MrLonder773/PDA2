// ════════════════════════════════════════════════════════
//  PDA 2 — pda2_platform_sim.cpp
//  Платформенный слой для PC-симулятора (SDL2).
//  Компилируется, только если PDA2_SIM определён (CMake сима,
//  -DPDA2_SIM) — в прошивке ESP32 тело пустое.
//
//  ⚠️ Этот файл нужен уже сейчас (чтобы PDA2.cpp собирался
//  одинаково на обеих платформах), но реально заработает
//  только когда появится сама сборка симулятора с SDL2 —
//  см. отдельную задачу по sim/.
// ════════════════════════════════════════════════════════

#include "pda2_platform.h"

#ifdef PDA2_SIM

#include <SDL2/SDL.h>
#include <cstdio>
#include <thread>
#include <chrono>

void pda2_platform_begin() {
    // На PC отдельного Serial нет — просто консоль.
    printf("PDA 2 SIM starting...\n");
}

void pda2_platform_i2c_begin(uint8_t /*sda*/, uint8_t /*scl*/) {
    // Реального I2C на PC нет — данные подставляют сами
    // PC-реализации Subsystem-классов (Rtc/Imu/... _sim.cpp).
}

uint32_t pda2_platform_now_ms() {
    return SDL_GetTicks();
}

void pda2_platform_sleep_ms(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

uint32_t pda2_platform_free_heap() {
    // Не критично на PC — заглушка, чтобы лог не падал на %u.
    return 0xFFFFFFFF;
}

uint32_t pda2_platform_free_psram() {
    return 0xFFFFFFFF;
}

// ── Touch: мышь через SDL2 ───────────────────────────────
// Координаты мыши уже в конечном логическом пространстве окна
// (окно = логический экран 1:1) — Touch_Class сам знает не
// применять поверх ещё и rotation-коррекцию (см. Touch_Class.cpp).
bool pda2_platform_touch_begin() {
    return true; // мышь есть всегда
}

bool pda2_platform_touch_read(int32_t& x, int32_t& y) {
    int mx = 0, my = 0;
    Uint32 buttons = SDL_GetMouseState(&mx, &my);
    x = mx;
    y = my;
    return (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
}

// ── Quit-флаг ─────────────────────────────────────────────
// Выставляется из Usb_Class_sim.cpp при виде SDL_QUIT в очереди
// событий (он же единственный, кто её вычитывает).
static bool _quit_requested = false;

void pda2_platform_request_quit() { _quit_requested = true; }

bool pda2_platform_should_quit() { return _quit_requested; }

#endif // PDA2_SIM