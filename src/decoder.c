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
        case OP_MOV_REG_MEM:   // MOV reg, [reg_base]
        case OP_MOV_MEM_REG:   // MOV [reg_base], reg
        case OP_ADD_REG_REG:
        case OP_SUB_REG_REG:
        case OP_CMP_REG_REG:
        case OP_AND_REG_REG:
        case OP_OR_REG_REG:
        case OP_XOR_REG_REG:
        case OP_TEST_REG_REG:
        case OP_SHL_REG_REG:
        case OP_SHR_REG_REG:
            inst->length = 3;
            if (!memory_read8(memory, eip + 1u, &inst->reg_a) ||
                !memory_read8(memory, eip + 2u, &inst->reg_b)) {
                return false;
            }
            return inst->reg_a < REG_COUNT && inst->reg_b < REG_COUNT;

        case OP_INC_REG:
        case OP_DEC_REG:
        case OP_NOT_REG:
        case OP_NEG_REG:
            inst->length = 2;
            if (!memory_read8(memory, eip + 1u, &inst->reg_a)) {
                return false;
            }
            return inst->reg_a < REG_COUNT;

        case OP_NOP:
        case OP_CLC:
        case OP_STC:
            inst->length = 1;
            return true;

        case OP_LOOP_REL8:
            inst->length = 2;
            uint8_t rel8;
            if (!memory_read8(memory, eip + 1u, &rel8)) {
                return false;
            }
            // Sign-extend 8-bit to 32-bit
            inst->imm32 = rel8;
            if (rel8 & 0x80) {
                inst->imm32 |= 0xFFFFFF00u;
            }
            return true;

        case OP_ADD_REG_IMM:
        case OP_SUB_REG_IMM:
        case OP_CMP_REG_IMM:
        case OP_AND_REG_IMM:
        case OP_OR_REG_IMM:
        case OP_XOR_REG_IMM:
            inst->length = 6;
            if (!memory_read8(memory, eip + 1u, &inst->reg_a) ||
                !memory_read32(memory, eip + 2u, &inst->imm32)) {
                return false;
            }
            return inst->reg_a < REG_COUNT;

        case OP_JMP_REL32:
        case OP_JZ_REL32:
        case OP_JNZ_REL32:
        case OP_JC_REL32:
        case OP_JNC_REL32:
        case OP_JS_REL32:
        case OP_JNS_REL32:
        case OP_JO_REL32:
        case OP_JNO_REL32:
        case OP_CALL_REL32:
            inst->length = 5;
            return memory_read32(memory, eip + 1u, &inst->imm32);

        case OP_RET:
            inst->length = 1;
            return true;

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
