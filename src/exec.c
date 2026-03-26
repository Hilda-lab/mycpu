#include "mycpu/exec.h"

#include "mycpu/isa.h"

static uint32_t *reg_ptr(cpu_t *cpu, uint8_t reg_id) {
    switch (reg_id) {
        case REG_EAX:
            return &cpu->eax;
        case REG_EBX:
            return &cpu->ebx;
        case REG_ECX:
            return &cpu->ecx;
        case REG_EDX:
            return &cpu->edx;
        case REG_ESI:
            return &cpu->esi;
        case REG_EDI:
            return &cpu->edi;
        case REG_EBP:
            return &cpu->ebp;
        case REG_ESP:
            return &cpu->esp;
        default:
            return NULL;
    }
}

static void set_zf(cpu_t *cpu, bool is_zero) {
    if (is_zero) {
        cpu->eflags |= FLAG_ZF;
    } else {
        cpu->eflags &= ~FLAG_ZF;
    }
}

static bool is_zf_set(const cpu_t *cpu) {
    return (cpu->eflags & FLAG_ZF) != 0;
}

static bool stack_push32(cpu_t *cpu, memory_t *memory, uint32_t value) {
    if (cpu->esp < 4u) {
        return false;
    }
    cpu->esp -= 4u;
    return memory_write32(memory, cpu->esp, value);
}

static bool stack_pop32(cpu_t *cpu, memory_t *memory, uint32_t *value) {
    if (!memory_read32(memory, cpu->esp, value)) {
        return false;
    }
    cpu->esp += 4u;
    return true;
}

void execute_instruction(cpu_t *cpu, memory_t *memory, const instruction_t *inst) {
    uint32_t *dst = NULL;
    uint32_t *src = NULL;
    uint32_t value = 0;
    int32_t rel = (int32_t)inst->imm32;
    uint32_t next_eip = cpu->eip + inst->length;

    switch (inst->opcode) {
        case OP_MOV_REG_IMM:
            dst = reg_ptr(cpu, inst->reg_a);
            if (dst == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            *dst = inst->imm32;
            cpu->eip = next_eip;
            return;

        case OP_MOV_REG_REG:
            dst = reg_ptr(cpu, inst->reg_a);
            src = reg_ptr(cpu, inst->reg_b);
            if (dst == NULL || src == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            *dst = *src;
            cpu->eip = next_eip;
            return;

        case OP_ADD_REG_REG:
            dst = reg_ptr(cpu, inst->reg_a);
            src = reg_ptr(cpu, inst->reg_b);
            if (dst == NULL || src == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            *dst += *src;
            set_zf(cpu, *dst == 0u);
            cpu->eip = next_eip;
            return;

        case OP_SUB_REG_REG:
            dst = reg_ptr(cpu, inst->reg_a);
            src = reg_ptr(cpu, inst->reg_b);
            if (dst == NULL || src == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            *dst -= *src;
            set_zf(cpu, *dst == 0u);
            cpu->eip = next_eip;
            return;

        case OP_CMP_REG_REG:
            dst = reg_ptr(cpu, inst->reg_a);
            src = reg_ptr(cpu, inst->reg_b);
            if (dst == NULL || src == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            set_zf(cpu, *dst == *src);
            cpu->eip = next_eip;
            return;

        case OP_JMP_REL32:
            cpu->eip = (uint32_t)((int32_t)next_eip + rel);
            return;

        case OP_JZ_REL32:
            cpu->eip = is_zf_set(cpu) ? (uint32_t)((int32_t)next_eip + rel) : next_eip;
            return;

        case OP_JNZ_REL32:
            cpu->eip = is_zf_set(cpu) ? next_eip : (uint32_t)((int32_t)next_eip + rel);
            return;

        case OP_PUSH_REG:
            src = reg_ptr(cpu, inst->reg_a);
            if (src == NULL || !stack_push32(cpu, memory, *src)) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            cpu->eip = next_eip;
            return;

        case OP_POP_REG:
            dst = reg_ptr(cpu, inst->reg_a);
            if (dst == NULL || !stack_pop32(cpu, memory, &value)) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            *dst = value;
            cpu->eip = next_eip;
            return;

        case OP_HLT:
            cpu->state = CPU_HALTED;
            cpu->eip = next_eip;
            return;

        default:
            cpu->state = CPU_EXCEPTION;
            return;
    }
}
