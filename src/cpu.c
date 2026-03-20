#include "mycpu/cpu.h"

void cpu_init(cpu_t *cpu) {
    cpu->eax = 0;
    cpu->ebx = 0;
    cpu->ecx = 0;
    cpu->edx = 0;
    cpu->esi = 0;
    cpu->edi = 0;
    cpu->ebp = 0;
    cpu->esp = 0;
    cpu->eip = 0;
    cpu->eflags = 0;
    cpu->state = CPU_STOPPED;
}

void cpu_reset(cpu_t *cpu, uint32_t entry) {
    cpu->eip = entry;
    cpu->state = CPU_RUNNING;
}

void cpu_step(cpu_t *cpu) {
    (void)cpu;
}
