#ifndef MYCPU_EXEC_H
#define MYCPU_EXEC_H

#include "mycpu/cpu.h"
#include "mycpu/decoder.h"
#include "mycpu/memory.h"

void execute_instruction(cpu_t *cpu, memory_t *memory, const instruction_t *inst);

#endif
