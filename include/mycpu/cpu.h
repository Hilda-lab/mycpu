#ifndef MYCPU_CPU_H
#define MYCPU_CPU_H

#include "mycpu/types.h"
#include "mycpu/memory.h"

typedef enum {
    CPU_STOPPED = 0,
    CPU_RUNNING,
    CPU_HALTED,
    CPU_EXCEPTION
} cpu_state_t;

typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t eip;
    uint32_t eflags;
    cpu_state_t state;
} cpu_t;

void cpu_init(cpu_t *cpu);
void cpu_reset(cpu_t *cpu, uint32_t entry, uint32_t stack_top);
void cpu_step(cpu_t *cpu, memory_t *memory);
void cpu_run(cpu_t *cpu, memory_t *memory, uint32_t max_steps);

#endif
