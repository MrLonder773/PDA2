#include "Memory_Class.h"

#ifndef PDA2_SIM

#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

static MemRegion _region(uint32_t caps) {
    MemRegion r;
    r.free     = (uint32_t)heap_caps_get_free_size(caps);
    r.largest  = (uint32_t)heap_caps_get_largest_free_block(caps);
    r.min_free = (uint32_t)heap_caps_get_minimum_free_size(caps);
    return r;
}

MemSnapshot Memory_Class::get() const {
    MemSnapshot s;
    s.internal = _region(MALLOC_CAP_INTERNAL);
    s.psram    = _region(MALLOC_CAP_SPIRAM);
    s.valid    = true;
    return s;
}

MemTask Memory_Class::task() const {
    MemTask t;
    t.core      = (int)xPortGetCoreID();
    t.priority  = (uint8_t)uxTaskPriorityGet(nullptr);
    t.name      = pcTaskGetName(nullptr);
    t.stack_min = (uint32_t)uxTaskGetStackHighWaterMark(nullptr);
    t.valid     = true;
    return t;
}

MemTaskList Memory_Class::tasks() const {
    MemTaskList l;
    TaskStatus_t st[PDA2_MEM_TASKS_MAX];
    const UBaseType_t n = uxTaskGetSystemState(st, PDA2_MEM_TASKS_MAX, nullptr);
    if (n == 0) return l;                       // пусто или массив мал → valid=false
    for (UBaseType_t i = 0; i < n; i++) {
        MemTaskInfo& t = l.items[i];
        strncpy(t.name, st[i].pcTaskName, sizeof(t.name) - 1);
        t.core      = (st[i].xCoreID == tskNO_AFFINITY) ? -1 : (int)st[i].xCoreID;
        t.priority  = (uint8_t)st[i].uxCurrentPriority;
        t.stack_min = (uint32_t)st[i].usStackHighWaterMark;
    }
    l.count = (uint8_t)n;
    l.valid = true;
    return l;
}

#endif // !PDA2_SIM