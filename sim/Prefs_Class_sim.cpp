// ════════════════════════════════════════════════════════
//  PDA 2 — Prefs_Class_sim.cpp
//  PC-замена NVS: плоский текстовый файл "ключ=значение" —
//  без вложенных папок, файл лежит рядом с исполняемым файлом
//  симулятора (текущая рабочая директория).
//
//  Ключи и дефолты — те же, что и в реальной NVS на железе
//  (см. Prefs_Class_esp32.cpp), для наглядности соответствия
//  при отладке.
//
//  Схема: всё состояние живёт в static-переменных в памяти;
//  begin() один раз читает файл поверх дефолтов; каждый setXxx()
//  сразу перезаписывает файл целиком. Не оптимально по числу
//  записей на диск, но это не флеш с ограниченным циклом
//  перезаписи (как NVS), а обычный SSD/HDD — не важно.
//
//  Компилируется, только если PDA2_SIM определён —
//  на ESP32 своя реализация (см. Prefs_Class_esp32.cpp).
// ════════════════════════════════════════════════════════

#include "Prefs_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"

#ifdef PDA2_SIM

#include <cstdio>
#include <cstring>
#include <cstdlib>   // atoi/atol — не тянутся транзитивно без Arduino.h

static const char* PREFS_FILE = "pda2_sim_prefs.txt";

// ── Состояние в памяти (дефолты — как на железе) ─────────
static uint8_t  _brightness   = 200;
static uint8_t  _rotation     = PDA2_ROTATION;
static uint8_t  _theme        = PDA2_THEME_DARK;
static uint32_t _sleep_ms     = PDA2_SLEEP_TIMEOUT_MS;
static int8_t   _last_app     = -1;
static bool     _wifi         = true;
static bool     _bt_en        = false;
static PdaStr32 _bt_last;        // "" по умолчанию
static uint8_t  _bt_last_t    = 1;
static PdaStr32 _bt_last_n;      // "" по умолчанию

// ── Сохранение всего набора разом ────────────────────────
static void _save() {
    FILE* f = fopen(PREFS_FILE, "w");
    if (!f) {
        PDA_LOGE("prefs", "SIM: failed to write %s", PREFS_FILE);
        return;
    }
    fprintf(f, "brightness=%u\n", _brightness);
    fprintf(f, "rotation=%u\n", _rotation);
    fprintf(f, "theme=%u\n", _theme);
    fprintf(f, "sleep_ms=%u\n", (unsigned)_sleep_ms);
    fprintf(f, "last_app=%d\n", _last_app);
    fprintf(f, "wifi=%d\n", _wifi ? 1 : 0);
    fprintf(f, "bt_en=%d\n", _bt_en ? 1 : 0);
    fprintf(f, "bt_last=%s\n", _bt_last.c_str());
    fprintf(f, "bt_last_t=%u\n", _bt_last_t);
    fprintf(f, "bt_last_n=%s\n", _bt_last_n.c_str());
    fclose(f);
}

// ── Загрузка поверх дефолтов ─────────────────────────────
void Prefs_Class::begin() {
    FILE* f = fopen(PREFS_FILE, "r");
    if (!f) {
        PDA_LOGI("prefs", "SIM: %s not found, using defaults", PREFS_FILE);
        return;
    }

    char line[96];
    while (fgets(line, sizeof(line), f)) {
        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char* key = line;
        char* val = eq + 1;
        char* nl = strchr(val, '\n');
        if (nl) *nl = '\0';

        if      (!strcmp(key, "brightness")) _brightness = (uint8_t)atoi(val);
        else if (!strcmp(key, "rotation"))   _rotation   = (uint8_t)atoi(val);
        else if (!strcmp(key, "theme"))      _theme      = (uint8_t)atoi(val);
        else if (!strcmp(key, "sleep_ms"))   _sleep_ms   = (uint32_t)atol(val);
        else if (!strcmp(key, "last_app"))   _last_app   = (int8_t)atoi(val);
        else if (!strcmp(key, "wifi"))       _wifi       = atoi(val) != 0;
        else if (!strcmp(key, "bt_en"))      _bt_en      = atoi(val) != 0;
        else if (!strcmp(key, "bt_last"))    _bt_last    = val;
        else if (!strcmp(key, "bt_last_t"))  _bt_last_t  = (uint8_t)atoi(val);
        else if (!strcmp(key, "bt_last_n"))  _bt_last_n  = val;
    }
    fclose(f);
    PDA_LOGI("prefs", "SIM: loaded %s", PREFS_FILE);
}

uint8_t Prefs_Class::getBrightness() { return _brightness; }
void    Prefs_Class::setBrightness(uint8_t v) { _brightness = v; _save(); }

uint8_t Prefs_Class::getRotation() { return _rotation; }
void    Prefs_Class::setRotation(uint8_t r) { _rotation = r; _save(); }

uint8_t Prefs_Class::getTheme() { return _theme; }
void    Prefs_Class::setTheme(uint8_t t) { _theme = t; _save(); }

uint32_t Prefs_Class::getSleepTimeout() { return _sleep_ms; }
void     Prefs_Class::setSleepTimeout(uint32_t ms) { _sleep_ms = ms; _save(); }

int8_t Prefs_Class::getLastApp() { return _last_app; }
void   Prefs_Class::setLastApp(int8_t id) { _last_app = id; _save(); }

bool Prefs_Class::getWifi() { return _wifi; }
void Prefs_Class::setWifi(bool val) { _wifi = val; _save(); }

bool     Prefs_Class::getBtEnabled()                    { return _bt_en; }
void     Prefs_Class::setBtEnabled(bool v)              { _bt_en = v; _save(); }
PdaStr32 Prefs_Class::getBtLastDevice()                 { return _bt_last; }
void     Prefs_Class::setBtLastDevice(const PdaStr32& m){ _bt_last = m.c_str(); _save(); }
uint8_t  Prefs_Class::getBtLastDeviceType()             { return _bt_last_t; }
void     Prefs_Class::setBtLastDeviceType(uint8_t t)    { _bt_last_t = t; _save(); }
PdaStr32 Prefs_Class::getBtLastDeviceName()                 { return _bt_last_n; }
void     Prefs_Class::setBtLastDeviceName(const PdaStr32& n){ _bt_last_n = n.c_str(); _save(); }

#endif // PDA2_SIM