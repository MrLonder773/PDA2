#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — pda2_platform.h
//  Единственное место, где PDA2.cpp расходится по платформам.
//
//  ESP32: pda2_platform_esp32.cpp (Arduino/ESP-IDF)
//  PC:    pda2_platform_sim.cpp   (сборка симулятора, -DPDA2_SIM)
//
//  Оба .cpp физически компилируются обеими сборками (PlatformIO
//  тянет весь lib/PDA2Core/src/), но каждый оборачивает своё тело
//  в #ifndef/#ifdef PDA2_SIM — реально собирается только один,
//  второй превращается в пустую единицу трансляции.
// ════════════════════════════════════════════════════════

#include <stdint.h>
#include <stddef.h>

// Serial + любая другая одноразовая платформенная инициализация при старте.
void pda2_platform_begin();

// I2C-шина. На ESP32 — Wire.begin(sda, scl) + setClock(100kHz).
// На PC — пусто, реального I2C нет (данные подставляют сами
// PC-реализации Subsystem-классов).
void pda2_platform_i2c_begin(uint8_t sda, uint8_t scl);

// Миллисекунды с старта. ESP32 — millis(). PC — SDL_GetTicks().
uint32_t pda2_platform_now_ms();

// Блокирующая пауза в мс. ESP32 — delay(). PC — свой sleep.
void pda2_platform_sleep_ms(uint32_t ms);

// Свободная память. На PC не критично — можно вернуть заглушку.
uint32_t pda2_platform_free_heap();
uint32_t pda2_platform_free_psram();

// Выделение буфера "как бы в PSRAM". ESP32 — реальная PSRAM
// (heap_caps_malloc + MALLOC_CAP_SPIRAM). PC — обычный malloc,
// PSRAM как отдельная концепция на PC не существует.
void* pda2_platform_psram_malloc(size_t size);

// Touch — начать/проверить наличие устройства.
// ESP32: сброс FT6236 через GPIO + проверка по I2C.
// PC: всегда true, мышь есть всегда.
bool pda2_platform_touch_begin();

// Touch — прочитать текущее состояние.
// ESP32: сырые координаты FT6236 (до коррекции rotation —
//        коррекцию делает Touch_Class::_transform()).
// PC: координаты мыши УЖЕ в конечном логическом пространстве
//     экрана (окно SDL и есть логический экран, корректировать
//     нечего) — Touch_Class сам знает, что на симе транформацию
//     по rotation применять не нужно.
// Возвращает true, если палец/кнопка мыши сейчас нажаты.
bool pda2_platform_touch_read(int32_t& x, int32_t& y);

// Запрошено ли завершение (крестик окна SDL). На ESP32 всегда false.
bool pda2_platform_should_quit();

// Запросить завершение — вызывается при виде SDL_QUIT
// (см. Usb_Class_sim.cpp, единственный, кто разбирает очередь событий).
// На ESP32 — пустая функция, вызывать неоткуда.
void pda2_platform_request_quit();