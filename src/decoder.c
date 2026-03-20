#include "mycpu/decoder.h"

bool decode_instruction(uint8_t raw, instruction_t *inst) {
    if (inst == NULL) {
        return false;
    }
    inst->opcode = raw;
    return true;
}
