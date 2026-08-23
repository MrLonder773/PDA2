// ════════════════════════════════════════════════════════
//  PDA 2 SIM — main.cpp
//  PC entry point (SDL2 + CMake). Mirrors src/main.cpp (ESP32):
//  PDA.begin() → Apps.add() → Apps.start() → loop → PDA.update().
//
//  Two things that look missing on purpose:
//   - SDL event pump (incl. window-close → quit) already lives
//     inside Usb_Class_sim.cpp (_platformPoll(), called from
//     Usb.update() inside PDA.update()) — no SDL_PollEvent here.
//   - lv_timer_handler() is already called every tick inside
//     Apps_Class::tick() (called from PDA.update()) — do not
//     call it again here, or LVGL will double-pump.
// ════════════════════════════════════════════════════════

#include <PDA2.h>
#include "pda2_platform.h"

#include "apps/clock/ClockApp.h"
#include "apps/accel/AccelApp.h"
#include "apps/gltest/GLTestApp.h"
#include "apps/bluetooth/BluetoothApp.h"

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    auto cfg = PDA.config();
    PDA.begin(cfg);

    PDA.Apps.add(new ClockApp());
    PDA.Apps.add(new AccelApp());
    PDA.Apps.add(new GLTestApp());
    PDA.Apps.add(new BluetoothApp());

    PDA.Apps.start();

    while (!pda2_platform_should_quit()) {
        PDA.update();
        pda2_platform_sleep_ms(5);
    }

    return 0;
}
