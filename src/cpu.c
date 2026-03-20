#include "mycpu/cpu.h"

#include "mycpu/decoder.h"
#include "mycpu/exec.h"

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

void cpu_reset(cpu_t *cpu, uint32_t entry, uint32_t stack_top) {
    cpu->eax = 0;
    cpu->ebx = 0;
    cpu->ecx = 0;
    cpu->edx = 0;
    cpu->esi = 0;
    cpu->edi = 0;
    cpu->ebp = 0;
    cpu->esp = stack_top;
    cpu->eip = entry;
    cpu->eflags = 0;
    cpu->state = CPU_RUNNING;
}

void cpu_step(cpu_t *cpu, memory_t *memory) {
    instruction_t inst;

    if (cpu == NULL || memory == NULL) {
        return;
    }
    if (cpu->state != CPU_RUNNING) {
        return;
    }
    if (!decode_instruction(memory, cpu->eip, &inst)) {
        cpu->state = CPU_EXCEPTION;
        return;
    }
    execute_instruction(cpu, memory, &inst);
}

void cpu_run(cpu_t *cpu, memory_t *memory, uint32_t max_steps) {
    uint32_t step = 0;
    while (cpu->state == CPU_RUNNING && step < max_steps) {
        cpu_step(cpu, memory);
        step++;
    }
}
