// ════════════════════════════════════════════════════════
//  PDA 2 — Fs_Class.cpp
//  Обёртка Fs_Class: делегирует в internal/sd (_esp32/_sim).
//  Единственная логика здесь — решение, можно ли форматировать
//  LittleFS при сбое монтирования.
// ════════════════════════════════════════════════════════

#include "Fs_Class.h"
#include "Prefs_Class.h"

#ifdef PDA2_FS_SELFTEST
void fs_selftest(Fs_Class& fs);   // Fs_SelfTest.cpp, только при -D PDA2_FS_SELFTEST
#endif

void Fs_Class::begin(Prefs_Class& prefs) {
    const bool known = prefs.getFsMounted();
    internal.begin(!known);
    if (internal.ok() && !known) prefs.setFsMounted(true);
    sd.begin();
#ifdef PDA2_FS_SELFTEST
    fs_selftest(*this);
#endif
}

bool Fs_Class::sdAvailable() {
    return sd.ok();
}