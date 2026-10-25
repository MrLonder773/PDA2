// ════════════════════════════════════════════════════════
//  PDA 2 — main.cpp
// ════════════════════════════════════════════════════════

#include <PDA2.h>

#include "apps/clock/ClockApp.h"
#include "apps/bluetooth/BluetoothApp.h"
#include "apps/tinygltest/TinyGLTestApp.h"

SET_LOOP_TASK_STACK_SIZE(32768);

void setup() {
    auto cfg = PDA.config();
    PDA.begin(cfg);
    
    PDA.Apps.add(new ClockApp());
    PDA.Apps.add(new BluetoothApp());
    PDA.Apps.add(new TinyGLTestApp());

    PDA.Apps.start();

    PDA.notify("Test(not App)", "test yvedomlenya, pishy latinitseq");
    PDA.notify("TestApp", "xaia! kak tebe yvedomlenye?");
    PDA.notify("ClockApp", "Test notification");
    PDA.notify("NotesApp", "File saved");
}

void loop() {
    PDA.update();
}