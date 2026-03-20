#ifndef MYCPU_DECODER_H
#define MYCPU_DECODER_H

#include "mycpu/types.h"

typedef struct {
    uint8_t opcode;
} instruction_t;

bool decode_instruction(uint8_t raw, instruction_t *inst);

#endif
