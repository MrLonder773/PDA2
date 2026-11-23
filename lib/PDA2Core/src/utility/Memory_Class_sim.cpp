#include "Memory_Class.h"

#ifdef PDA2_SIM

// На PC счётчиков кучи PSRAM/internal и задач FreeRTOS нет: valid=false.
MemSnapshot Memory_Class::get()  const { return MemSnapshot{}; }
MemTask     Memory_Class::task() const { return MemTask{}; }
MemTaskList Memory_Class::tasks() const { return MemTaskList{}; }

#endif // PDA2_SIM