#include "mycpu/exec.h"

#include "mycpu/isa.h"
#include "mycpu/cpu.h"

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

static void set_sf(cpu_t *cpu, uint32_t result) {
    if (result & 0x80000000u) {
        cpu->eflags |= FLAG_SF;
    } else {
        cpu->eflags &= ~FLAG_SF;
    }
}

static void set_pf(cpu_t *cpu, uint8_t low_byte) {
    int count = 0;
    for (int i = 0; i < 8; i++) {
        if (low_byte & (1u << i)) count++;
    }
    if ((count & 1) == 0) {
        cpu->eflags |= FLAG_PF;
    } else {
        cpu->eflags &= ~FLAG_PF;
    }
}

static void set_cf(cpu_t *cpu, bool carry) {
    if (carry) {
        cpu->eflags |= FLAG_CF;
    } else {
        cpu->eflags &= ~FLAG_CF;
    }
}

static void set_of_add(cpu_t *cpu, uint32_t a, uint32_t b, uint32_t result) {
    bool a_sign = (a & 0x80000000u) != 0;
    bool b_sign = (b & 0x80000000u) != 0;
    bool r_sign = (result & 0x80000000u) != 0;
    if (a_sign == b_sign && a_sign != r_sign) {
        cpu->eflags |= FLAG_OF;
    } else {
        cpu->eflags &= ~FLAG_OF;
    }
}

static void set_of_sub(cpu_t *cpu, uint32_t a, uint32_t b, uint32_t result) {
    bool a_sign = (a & 0x80000000u) != 0;
    bool b_sign = (b & 0x80000000u) != 0;
    bool r_sign = (result & 0x80000000u) != 0;
    if (a_sign != b_sign && a_sign != r_sign) {
        cpu->eflags |= FLAG_OF;
    } else {
        cpu->eflags &= ~FLAG_OF;
    }
}

static void update_flags_logical(cpu_t *cpu, uint32_t result) {
    set_zf(cpu, result == 0u);
    set_sf(cpu, result);
    set_pf(cpu, (uint8_t)result);
    cpu->eflags &= ~FLAG_CF;
    cpu->eflags &= ~FLAG_OF;
}

static void update_flags_add(cpu_t *cpu, uint32_t a, uint32_t b, uint32_t result) {
    set_zf(cpu, result == 0u);
    set_sf(cpu, result);
    set_pf(cpu, (uint8_t)result);
    set_cf(cpu, result < a);
    set_of_add(cpu, a, b, result);
}

static void update_flags_sub(cpu_t *cpu, uint32_t a, uint32_t b, uint32_t result) {
    set_zf(cpu, result == 0u);
    set_sf(cpu, result);
    set_pf(cpu, (uint8_t)result);
    set_cf(cpu, a < b);
    set_of_sub(cpu, a, b, result);
}

static bool is_zf_set(const cpu_t *cpu) {
    return (cpu->eflags & FLAG_ZF) != 0;
}

static bool is_cf_set(const cpu_t *cpu) {
    return (cpu->eflags & FLAG_CF) != 0;
}

static bool is_sf_set(const cpu_t *cpu) {
    return (cpu->eflags & FLAG_SF) != 0;
}

static bool is_of_set(const cpu_t *cpu) {
    return (cpu->eflags & FLAG_OF) != 0;
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

        case OP_MOV_REG_MEM: {
            // MOV reg_a, [reg_b] - 从内存地址读取到寄存器
            dst = reg_ptr(cpu, inst->reg_a);
            src = reg_ptr(cpu, inst->reg_b);
            if (dst == NULL || src == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            uint32_t addr = *src;
            if (!memory_read32(memory, addr, dst)) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            cpu->eip = next_eip;
            return;
        }

        case OP_MOV_MEM_REG: {
            // MOV [reg_a], reg_b - 将寄存器值写入内存地址
            dst = reg_ptr(cpu, inst->reg_a);
            src = reg_ptr(cpu, inst->reg_b);
            if (dst == NULL || src == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            uint32_t addr = *dst;
            if (!memory_write32(memory, addr, *src)) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            cpu->eip = next_eip;
            return;
        }

        case OP_ADD_REG_REG: {
            dst = reg_ptr(cpu, inst->reg_a);
            src = reg_ptr(cpu, inst->reg_b);
            if (dst == NULL || src == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            uint32_t a = *dst;
            uint32_t b = *src;
            uint32_t result = a + b;
            *dst = result;
            update_flags_add(cpu, a, b, result);
            cpu->eip = next_eip;
            return;
        }

        case OP_ADD_REG_IMM: {
            dst = reg_ptr(cpu, inst->reg_a);
            if (dst == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            uint32_t a = *dst;
            uint32_t b = inst->imm32;
            uint32_t result = a + b;
            *dst = result;
            update_flags_add(cpu, a, b, result);
            cpu->eip = next_eip;
            return;
        }

        case OP_SUB_REG_REG: {
            dst = reg_ptr(cpu, inst->reg_a);
            src = reg_ptr(cpu, inst->reg_b);
            if (dst == NULL || src == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            uint32_t a = *dst;
            uint32_t b = *src;
            uint32_t result = a - b;
            *dst = result;
            update_flags_sub(cpu, a, b, result);
            cpu->eip = next_eip;
            return;
        }

        case OP_SUB_REG_IMM: {
            dst = reg_ptr(cpu, inst->reg_a);
            if (dst == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            uint32_t a = *dst;
            uint32_t b = inst->imm32;
            uint32_t result = a - b;
            *dst = result;
            update_flags_sub(cpu, a, b, result);
            cpu->eip = next_eip;
            return;
        }

        case OP_CMP_REG_REG: {
            dst = reg_ptr(cpu, inst->reg_a);
            src = reg_ptr(cpu, inst->reg_b);
            if (dst == NULL || src == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            uint32_t a = *dst;
            uint32_t b = *src;
            uint32_t result = a - b;
            update_flags_sub(cpu, a, b, result);
            cpu->eip = next_eip;
            return;
        }

        case OP_CMP_REG_IMM: {
            dst = reg_ptr(cpu, inst->reg_a);
            if (dst == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            uint32_t a = *dst;
            uint32_t b = inst->imm32;
            uint32_t result = a - b;
            update_flags_sub(cpu, a, b, result);
            cpu->eip = next_eip;
            return;
        }

        case OP_INC_REG: {
            dst = reg_ptr(cpu, inst->reg_a);
            if (dst == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            uint32_t a = *dst;
            uint32_t result = a + 1;
            *dst = result;
            // INC does not affect CF
            set_zf(cpu, result == 0u);
            set_sf(cpu, result);
            set_pf(cpu, (uint8_t)result);
            set_of_add(cpu, a, 1, result);
            cpu->eip = next_eip;
            return;
        }

        case OP_DEC_REG: {
            dst = reg_ptr(cpu, inst->reg_a);
            if (dst == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            uint32_t a = *dst;
            uint32_t result = a - 1;
            *dst = result;
            // DEC does not affect CF
            set_zf(cpu, result == 0u);
            set_sf(cpu, result);
            set_pf(cpu, (uint8_t)result);
            set_of_sub(cpu, a, 1, result);
            cpu->eip = next_eip;
            return;
        }

        case OP_AND_REG_REG: {
            dst = reg_ptr(cpu, inst->reg_a);
            src = reg_ptr(cpu, inst->reg_b);
            if (dst == NULL || src == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            *dst &= *src;
            update_flags_logical(cpu, *dst);
            cpu->eip = next_eip;
            return;
        }

        case OP_AND_REG_IMM: {
            dst = reg_ptr(cpu, inst->reg_a);
            if (dst == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            *dst &= inst->imm32;
            update_flags_logical(cpu, *dst);
            cpu->eip = next_eip;
            return;
        }

        case OP_OR_REG_REG: {
            dst = reg_ptr(cpu, inst->reg_a);
            src = reg_ptr(cpu, inst->reg_b);
            if (dst == NULL || src == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            *dst |= *src;
            update_flags_logical(cpu, *dst);
            cpu->eip = next_eip;
            return;
        }

        case OP_OR_REG_IMM: {
            dst = reg_ptr(cpu, inst->reg_a);
            if (dst == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            *dst |= inst->imm32;
            update_flags_logical(cpu, *dst);
            cpu->eip = next_eip;
            return;
        }

        case OP_XOR_REG_REG: {
            dst = reg_ptr(cpu, inst->reg_a);
            src = reg_ptr(cpu, inst->reg_b);
            if (dst == NULL || src == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            *dst ^= *src;
            update_flags_logical(cpu, *dst);
            cpu->eip = next_eip;
            return;
        }

        case OP_XOR_REG_IMM: {
            dst = reg_ptr(cpu, inst->reg_a);
            if (dst == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            *dst ^= inst->imm32;
            update_flags_logical(cpu, *dst);
            cpu->eip = next_eip;
            return;
        }

        case OP_NOT_REG: {
            dst = reg_ptr(cpu, inst->reg_a);
            if (dst == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            *dst = ~(*dst);
            // NOT does not affect any flags
            cpu->eip = next_eip;
            return;
        }

        case OP_TEST_REG_REG: {
            dst = reg_ptr(cpu, inst->reg_a);
            src = reg_ptr(cpu, inst->reg_b);
            if (dst == NULL || src == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            uint32_t result = *dst & *src;
            update_flags_logical(cpu, result);
            cpu->eip = next_eip;
            return;
        }

        case OP_SHL_REG_REG: {
            dst = reg_ptr(cpu, inst->reg_a);
            src = reg_ptr(cpu, inst->reg_b);
            if (dst == NULL || src == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            uint8_t count = (uint8_t)(*src & 0x1F);
            if (count > 0) {
                uint32_t result = *dst << count;
                set_cf(cpu, (*dst >> (32 - count)) & 1);
                *dst = result;
                update_flags_logical(cpu, *dst);
                set_cf(cpu, (*dst >> (32 - count)) & 1);
            }
            cpu->eip = next_eip;
            return;
        }

        case OP_SHR_REG_REG: {
            dst = reg_ptr(cpu, inst->reg_a);
            src = reg_ptr(cpu, inst->reg_b);
            if (dst == NULL || src == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            uint8_t count = (uint8_t)(*src & 0x1F);
            if (count > 0) {
                set_cf(cpu, (*dst >> (count - 1)) & 1);
                *dst = *dst >> count;
                update_flags_logical(cpu, *dst);
            }
            cpu->eip = next_eip;
            return;
        }

        case OP_NEG_REG: {
            dst = reg_ptr(cpu, inst->reg_a);
            if (dst == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            uint32_t a = *dst;
            uint32_t result = ~a + 1;  // Two's complement negation
            *dst = result;
            // NEG sets flags like SUB 0, a
            update_flags_sub(cpu, 0, a, result);
            // Special case: if a was 0, CF is cleared
            if (a == 0) {
                cpu->eflags &= ~FLAG_CF;
            } else {
                cpu->eflags |= FLAG_CF;
            }
            cpu->eip = next_eip;
            return;
        }

        case OP_NOP:
            cpu->eip = next_eip;
            return;

        case OP_CLC:
            cpu->eflags &= ~FLAG_CF;
            cpu->eip = next_eip;
            return;

        case OP_STC:
            cpu->eflags |= FLAG_CF;
            cpu->eip = next_eip;
            return;

        case OP_LOOP_REL8: {
            // LOOP: ECX--, if ECX != 0 then jump
            uint32_t *ecx = reg_ptr(cpu, REG_ECX);
            if (ecx == NULL) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            (*ecx)--;
            if (*ecx != 0) {
                cpu->eip = (uint32_t)((int32_t)next_eip + rel);
            } else {
                cpu->eip = next_eip;
            }
            return;
        }

        case OP_JMP_REL32:
            cpu->eip = (uint32_t)((int32_t)next_eip + rel);
            return;

        case OP_JZ_REL32:
            cpu->eip = is_zf_set(cpu) ? (uint32_t)((int32_t)next_eip + rel) : next_eip;
            return;

        case OP_JNZ_REL32:
            cpu->eip = is_zf_set(cpu) ? next_eip : (uint32_t)((int32_t)next_eip + rel);
            return;

        case OP_JC_REL32:
            cpu->eip = is_cf_set(cpu) ? (uint32_t)((int32_t)next_eip + rel) : next_eip;
            return;

        case OP_JNC_REL32:
            cpu->eip = is_cf_set(cpu) ? next_eip : (uint32_t)((int32_t)next_eip + rel);
            return;

        case OP_JS_REL32:
            cpu->eip = is_sf_set(cpu) ? (uint32_t)((int32_t)next_eip + rel) : next_eip;
            return;

        case OP_JNS_REL32:
            cpu->eip = is_sf_set(cpu) ? next_eip : (uint32_t)((int32_t)next_eip + rel);
            return;

        case OP_JO_REL32:
            cpu->eip = is_of_set(cpu) ? (uint32_t)((int32_t)next_eip + rel) : next_eip;
            return;

        case OP_JNO_REL32:
            cpu->eip = is_of_set(cpu) ? next_eip : (uint32_t)((int32_t)next_eip + rel);
            return;

        case OP_CALL_REL32:
            if (!stack_push32(cpu, memory, next_eip)) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            cpu->eip = (uint32_t)((int32_t)next_eip + rel);
            return;

        case OP_RET:
            if (!stack_pop32(cpu, memory, &value)) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            cpu->eip = value;
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

        case OP_PUSHF:
            // 标志寄存器入栈
            if (!stack_push32(cpu, memory, cpu->eflags)) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            cpu->eip = next_eip;
            return;

        case OP_POPF:
            // 从栈中弹出标志寄存器
            if (!stack_pop32(cpu, memory, &value)) {
                cpu->state = CPU_EXCEPTION;
                return;
            }
            cpu->eflags = value;
            // 同步中断使能标志
            cpu->interrupts_enabled = (value & EFLAGS_IF) != 0;
            cpu->eip = next_eip;
            return;

        case OP_INT_IMM8:
            // 软中断
            cpu_interrupt(cpu, memory, inst->imm8, 0);
            return;

        case OP_IRET:
            // 中断返回
            cpu_iret(cpu, memory);
            return;

        case OP_CLI:
            // 禁用中断
            cpu_disable_interrupts(cpu);
            cpu->eip = next_eip;
            return;

        case OP_STI:
            // 启用中断
            cpu_enable_interrupts(cpu);
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
