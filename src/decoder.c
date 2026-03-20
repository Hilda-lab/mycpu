#include "mycpu/decoder.h"

#include <string.h>

#include "mycpu/isa.h"

bool decode_instruction(const memory_t *memory, uint32_t eip, instruction_t *inst) {
    if (inst == NULL) {
        return false;
    }

    memset(inst, 0, sizeof(*inst));
    if (!memory_read8(memory, eip, &inst->opcode)) {
        return false;
    }

    switch (inst->opcode) {
        case OP_MOV_REG_IMM:
            inst->length = 6;
            if (!memory_read8(memory, eip + 1u, &inst->reg_a) ||
                !memory_read32(memory, eip + 2u, &inst->imm32)) {
                return false;
            }
            return inst->reg_a < REG_COUNT;

        case OP_MOV_REG_REG:
        case OP_ADD_REG_REG:
        case OP_SUB_REG_REG:
        case OP_CMP_REG_REG:
            inst->length = 3;
            if (!memory_read8(memory, eip + 1u, &inst->reg_a) ||
                !memory_read8(memory, eip + 2u, &inst->reg_b)) {
                return false;
            }
            return inst->reg_a < REG_COUNT && inst->reg_b < REG_COUNT;

        case OP_JMP_REL32:
        case OP_JZ_REL32:
        case OP_JNZ_REL32:
            inst->length = 5;
            return memory_read32(memory, eip + 1u, &inst->imm32);

        case OP_PUSH_REG:
        case OP_POP_REG:
            inst->length = 2;
            if (!memory_read8(memory, eip + 1u, &inst->reg_a)) {
                return false;
            }
            return inst->reg_a < REG_COUNT;

        case OP_HLT:
            inst->length = 1;
            return true;

        default:
            return false;
    }
}
