#include "Memory_Class.h"
#include "../pda2_log.h"

void Memory_Class::log(const char* tag) const {
#ifdef PDA2_MEMLOG
    const MemSnapshot m = get();
    const MemTask     t = task();
    if (!m.valid || !t.valid) return;
    PDA_LOGI("mem", "%s | int free %u largest %u min %u | psram free %u largest %u min %u | core %d prio %u task %s stack-min %u",
             tag,
             (unsigned)m.internal.free, (unsigned)m.internal.largest, (unsigned)m.internal.min_free,
             (unsigned)m.psram.free,    (unsigned)m.psram.largest,    (unsigned)m.psram.min_free,
             t.core, (unsigned)t.priority, t.name, (unsigned)t.stack_min);
#else
    (void)tag;
#endif
}

void Memory_Class::logTasks(const char* tag) const {
#ifdef PDA2_MEMLOG
    const MemTaskList l = tasks();
    if (!l.valid) { PDA_LOGW("mem", "%s | tasks: no data", tag); return; }
    for (uint8_t i = 0; i < l.count; i++) {
        const MemTaskInfo& t = l.items[i];
        if (t.core < 0) PDA_LOGI("mem", "%s | task %-12s core any prio %u stack-min %u", tag, t.name, (unsigned)t.priority, (unsigned)t.stack_min);
        else            PDA_LOGI("mem", "%s | task %-12s core %d prio %u stack-min %u",   tag, t.name, t.core, (unsigned)t.priority, (unsigned)t.stack_min);
    }
#else
    (void)tag;
#endif
}