// ════════════════════════════════════════════════════════
//  PDA 2 — main.cpp
// ════════════════════════════════════════════════════════

#include <PDA2.h>
#include "apps/clock/ClockApp.h"
// #include "apps/notes/NotesApp.h"
// #include "apps/files/FilesApp.h"
#include "apps/accel/AccelApp.h"
// #include "apps/settings/SettingsApp.h"
// #include "apps/walkie/WalkieApp.h"
#include "apps/gltest/GLTestApp.h"
#include "apps/bluetooth/BluetoothApp.h"

SET_LOOP_TASK_STACK_SIZE(32768);

void setup() {
    auto cfg = PDA.config();
    PDA.begin(cfg);
    PDA.Apps.add(new ClockApp());
    // PDA.Apps.add(new NotesApp());
    // PDA.Apps.add(new FilesApp());
    PDA.Apps.add(new AccelApp());
    // PDA.Apps.add(new SettingsApp());
    // PDA.Apps.add(new WalkieApp());
    PDA.Apps.add(new GLTestApp());
    PDA.Apps.add(new BluetoothApp());

    PDA.notify("Test(not App)", "test yvedomlenya, pishy latinitseq");
    PDA.notify("TestApp", "xaia! kak tebe yvedomlenye?");

    PDA.Apps.start();

    PDA.notify("ClockApp", "Test notification");
    PDA.notify("NotesApp", "File saved");
}

void loop() {
    PDA.update();
}