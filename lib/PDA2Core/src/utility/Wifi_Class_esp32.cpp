// ════════════════════════════════════════════════════════
//  PDA 2 — Wifi_Class_esp32.cpp
//
//  Компилируется, только если PDA2_SIM НЕ определён —
//  в сборке симулятора тело пустое (см. Wifi_Class_sim.cpp).
// ════════════════════════════════════════════════════════

#include "Wifi_Class.h"

#ifndef PDA2_SIM

#include <WiFi.h>
#include "../pda2_log.h"

void Wifi_Class::setEnabled(bool on) {
    WiFi.mode(on ? WIFI_STA : WIFI_OFF);
    PDA_LOGI("wifi", "WiFi %s", on ? "on" : "off");
}

#endif // !PDA2_SIM