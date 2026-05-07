// ════════════════════════════════════════════════════════
//  PDA 2 — main.cpp
// ════════════════════════════════════════════════════════

#include <PDA2.h>
#include "apps/clock/ClockApp.h"
#include "apps/notes/NotesApp.h"
// #include "apps/files/FilesApp.h"
#include "apps/accel/AccelApp.h"
// #include "apps/settings/SettingsApp.h"

void setup() {
    auto cfg = PDA.config();
    PDA.begin(cfg);

    PDA.Apps.add(new ClockApp());
    PDA.Apps.add(new NotesApp());
    // PDA.Apps.add(new FilesApp());
    PDA.Apps.add(new AccelApp());
    // PDA.Apps.add(new SettingsApp());

    PDA.Apps.start();
}

void loop() {
    PDA.update();
}