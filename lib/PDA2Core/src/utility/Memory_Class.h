#pragma once
// ════════════════════════════════════════════════════════
//  PDA 2 — Memory_Class.h
//  Учёт памяти и задач. Данные — get()/task() (valid: false = нет данных),
//  log(tag) — одна строка в лог, активна только с -D PDA2_MEMLOG.
//  task() и log() описывают ВЫЗЫВАЮЩУЮ задачу — звать прямо из неё.
// ════════════════════════════════════════════════════════

#include "../pda2_config.h"
#include <stdint.h>

struct MemRegion {
    uint32_t free     = 0;
    uint32_t largest  = 0;   // наибольший свободный блок
    uint32_t min_free = 0;   // минимум free за всё время работы
};

struct MemSnapshot {
    bool      valid = false;
    MemRegion internal;
    MemRegion psram;
};

struct MemTask {
    bool        valid     = false;
    int         core      = -1;
    uint8_t     priority  = 0;
    const char* name      = "";
    uint32_t    stack_min = 0;   // минимальный запас стека за всё время
};

struct MemTaskInfo {
    char     name[16] = {};
    int      core      = -1;   // -1 = без привязки к ядру (привязка, а не текущее ядро)
    uint8_t  priority  = 0;
    uint32_t stack_min = 0;    // минимальный запас стека за всё время, байт
};

struct MemTaskList {
    bool        valid = false;  // false: нет данных или задач больше PDA2_MEM_TASKS_MAX
    uint8_t     count = 0;
    MemTaskInfo items[PDA2_MEM_TASKS_MAX];
};

class Memory_Class {
public:
    MemSnapshot get()  const;
    MemTask     task() const;
    void        log(const char* tag) const;
    MemTaskList tasks() const;
    void        logTasks(const char* tag) const;
};