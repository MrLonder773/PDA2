// ════════════════════════════════════════════════════════
//  PDA 2 SIM — main.cpp
//  Точка входа на PC (SDL2 + CMake). Повторяет src/main.cpp (ESP32):
//  PDA.begin() → Apps.add() → Apps.start() → цикл → PDA.update().
//
//  Две вещи, которые выглядят пропущенными — это намеренно:
//   - Опрос событий SDL (в т.ч. закрытие окна → выход) уже живёт
//     внутри Usb_Class_sim.cpp (_platformPoll(), вызывается из
//     Usb.update() внутри PDA.update()) — SDL_PollEvent здесь не нужен.
//   - lv_timer_handler() уже вызывается каждый тик внутри
//     Apps_Class::tick() (вызывается из PDA.update()) — не вызывать
//     повторно здесь, иначе LVGL будет качать дважды.
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