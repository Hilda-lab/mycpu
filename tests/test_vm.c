#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "mycpu/cpu.h"
#include "mycpu/isa.h"
#include "mycpu/memory.h"

static void write_program(memory_t *memory, const uint8_t *program, size_t size) {
    for (size_t i = 0; i < size; i++) {
        bool ok = memory_write8(memory, (uint32_t)i, program[i]);
        assert(ok);
    }
}

static void run_program(cpu_t *cpu, memory_t *memory, const uint8_t *program, size_t size, uint32_t max_steps) {
    write_program(memory, program, size);
    cpu_init(cpu);
    cpu_reset(cpu, 0u, (uint32_t)memory->size);
    cpu_run(cpu, memory, max_steps);
}

static void test_cmp_jz_taken(void) {
    memory_t memory;
    cpu_t cpu;
    uint8_t program[] = {
        OP_MOV_REG_IMM, REG_EAX, 0x05, 0x00, 0x00, 0x00,
        OP_MOV_REG_IMM, REG_EBX, 0x05, 0x00, 0x00, 0x00,
        OP_CMP_REG_REG, REG_EAX, REG_EBX,
        OP_JZ_REL32, 0x06, 0x00, 0x00, 0x00,
        OP_MOV_REG_IMM, REG_ECX, 0x00, 0x00, 0x00, 0x00,
        OP_MOV_REG_IMM, REG_ECX, 0x01, 0x00, 0x00, 0x00,
        OP_HLT
    };

    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    run_program(&cpu, &memory, program, sizeof(program), 64u);

    assert(cpu.state == CPU_HALTED);
    assert(cpu.ecx == 1u);
    assert((cpu.eflags & FLAG_ZF) != 0u);
    memory_free(&memory);
}

static void test_stack_add_sub_jnz(void) {
    memory_t memory;
    cpu_t cpu;
    uint32_t original_esp = 0;
    uint8_t program[] = {
        OP_MOV_REG_IMM, REG_EAX, 0x02, 0x00, 0x00, 0x00,
        OP_PUSH_REG, REG_EAX,
        OP_MOV_REG_IMM, REG_EAX, 0x0A, 0x00, 0x00, 0x00,
        OP_POP_REG, REG_EBX,
        OP_ADD_REG_REG, REG_EAX, REG_EBX,
        OP_SUB_REG_REG, REG_EAX, REG_EBX,
        OP_CMP_REG_REG, REG_EAX, REG_EBX,
        OP_JNZ_REL32, 0x06, 0x00, 0x00, 0x00,
        OP_MOV_REG_IMM, REG_EDX, 0x00, 0x00, 0x00, 0x00,
        OP_MOV_REG_IMM, REG_EDX, 0x07, 0x00, 0x00, 0x00,
        OP_HLT
    };

    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    cpu_init(&cpu);
    cpu_reset(&cpu, 0u, (uint32_t)memory.size);
    original_esp = cpu.esp;

    write_program(&memory, program, sizeof(program));
    cpu_run(&cpu, &memory, 128u);

    assert(cpu.state == CPU_HALTED);
    assert(cpu.eax == 10u);
    assert(cpu.ebx == 2u);
    assert(cpu.edx == 7u);
    assert(cpu.esp == original_esp);
    memory_free(&memory);
}

static void test_jmp_skip(void) {
    memory_t memory;
    cpu_t cpu;
    uint8_t program[] = {
        OP_MOV_REG_IMM, REG_EAX, 0x01, 0x00, 0x00, 0x00,
        OP_JMP_REL32, 0x06, 0x00, 0x00, 0x00,
        OP_MOV_REG_IMM, REG_EAX, 0x02, 0x00, 0x00, 0x00,
        OP_HLT
    };

    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    run_program(&cpu, &memory, program, sizeof(program), 64u);

    assert(cpu.state == CPU_HALTED);
    assert(cpu.eax == 1u);
    memory_free(&memory);
}

int main(void) {
    test_cmp_jz_taken();
    test_stack_add_sub_jnz();
    test_jmp_skip();
    return 0;
}