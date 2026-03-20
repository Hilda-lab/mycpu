#ifndef MYCPU_EXEC_H
#define MYCPU_EXEC_H

#include "mycpu/cpu.h"
#include "mycpu/decoder.h"

void execute_instruction(cpu_t *cpu, const instruction_t *inst);

#endif
