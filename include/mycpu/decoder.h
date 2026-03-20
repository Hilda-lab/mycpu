#ifndef MYCPU_DECODER_H
#define MYCPU_DECODER_H

#include "mycpu/memory.h"
#include "mycpu/types.h"

typedef struct {
    uint8_t opcode;
    uint8_t reg_a;
    uint8_t reg_b;
    uint32_t imm32;
    uint8_t length;
} instruction_t;

bool decode_instruction(const memory_t *memory, uint32_t eip, instruction_t *inst);

#endif
