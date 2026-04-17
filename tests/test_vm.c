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

// 测试 AND, OR, XOR 指令
static void test_logical_operations(void) {
    memory_t memory;
    cpu_t cpu;
    // EAX = 0xFF; EBX = 0x0F
    // EAX = EAX & EBX  -> EAX = 0x0F
    // ECX = EAX | EBX  -> ECX = 0x0F
    // EDX = 0x0F; EDX = EDX ^ EAX  -> EDX = 0x00, ZF=1
    uint8_t program[] = {
        OP_MOV_REG_IMM, REG_EAX, 0xFF, 0x00, 0x00, 0x00,
        OP_MOV_REG_IMM, REG_EBX, 0x0F, 0x00, 0x00, 0x00,
        OP_AND_REG_REG, REG_EAX, REG_EBX,     // EAX = 0x0F
        OP_OR_REG_REG, REG_ECX, REG_EAX,      // ECX = 0x0F
        OP_MOV_REG_IMM, REG_EDX, 0x0F, 0x00, 0x00, 0x00, // EDX = 0x0F
        OP_XOR_REG_REG, REG_EDX, REG_EAX,     // EDX = 0x0F ^ 0x0F = 0
        OP_HLT
    };

    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    run_program(&cpu, &memory, program, sizeof(program), 64u);

    assert(cpu.state == CPU_HALTED);
    assert(cpu.eax == 0x0Fu);  // 0xFF & 0x0F = 0x0F
    assert(cpu.ecx == 0x0Fu);  // 0 | 0x0F = 0x0F
    assert(cpu.edx == 0x00u);  // 0x0F ^ 0x0F = 0
    assert((cpu.eflags & FLAG_ZF) != 0u);  // EDX=0, ZF=1
    memory_free(&memory);
}

// 测试 NOT, INC, DEC 指令
static void test_not_inc_dec(void) {
    memory_t memory;
    cpu_t cpu;
    // EAX = 0; NOT EAX -> EAX = 0xFFFFFFFF
    // INC EAX -> EAX = 0, ZF=1
    // EBX = 1; DEC EBX -> EBX = 0, ZF=1
    uint8_t program[] = {
        OP_MOV_REG_IMM, REG_EAX, 0x00, 0x00, 0x00, 0x00,
        OP_NOT_REG, REG_EAX,                    // EAX = 0xFFFFFFFF
        OP_INC_REG, REG_EAX,                    // EAX = 0
        OP_MOV_REG_IMM, REG_EBX, 0x01, 0x00, 0x00, 0x00,
        OP_DEC_REG, REG_EBX,                    // EBX = 0
        OP_HLT
    };

    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    run_program(&cpu, &memory, program, sizeof(program), 64u);

    assert(cpu.state == CPU_HALTED);
    assert(cpu.eax == 0u);
    assert(cpu.ebx == 0u);
    assert((cpu.eflags & FLAG_ZF) != 0u);  // DEC 0 -> ZF=1
    memory_free(&memory);
}

// 测试立即数算术指令
static void test_imm_arithmetic(void) {
    memory_t memory;
    cpu_t cpu;
    // EAX = 100; ADD EAX, 50 -> EAX = 150
    // SUB EAX, 30 -> EAX = 120
    // AND EAX, 0xFF -> EAX = 120
    // OR EAX, 0x100 -> EAX = 0x178 = 376
    // XOR EAX, 0x78 -> EAX = 0x100 = 256
    uint8_t program[] = {
        OP_MOV_REG_IMM, REG_EAX, 100, 0x00, 0x00, 0x00,
        OP_ADD_REG_IMM, REG_EAX, 50, 0x00, 0x00, 0x00,
        OP_SUB_REG_IMM, REG_EAX, 30, 0x00, 0x00, 0x00,
        OP_AND_REG_IMM, REG_EAX, 0xFF, 0x00, 0x00, 0x00,
        OP_OR_REG_IMM, REG_EAX, 0x00, 0x01, 0x00, 0x00,
        OP_XOR_REG_IMM, REG_EAX, 0x78, 0x00, 0x00, 0x00,
        OP_HLT
    };

    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    run_program(&cpu, &memory, program, sizeof(program), 64u);

    assert(cpu.state == CPU_HALTED);
    assert(cpu.eax == 0x100u);  // 256
    memory_free(&memory);
}

// 测试 CALL/RET 指令
static void test_call_ret(void) {
    memory_t memory;
    cpu_t cpu;
    // CALL function
    // MOV EAX, 100 (return here after RET)
    // HLT
    // function:
    //   MOV EBX, 200
    //   RET
    uint8_t program[] = {
        OP_CALL_REL32, 0x07, 0x00, 0x00, 0x00,    // CALL +7 (跳到 MOV EBX, 200)
        OP_MOV_REG_IMM, REG_EAX, 100, 0x00, 0x00, 0x00,  // 返回点
        OP_HLT,
        // function:
        OP_MOV_REG_IMM, REG_EBX, 200, 0x00, 0x00, 0x00,  // 地址 18
        OP_RET                                         // 地址 24
    };

    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    run_program(&cpu, &memory, program, sizeof(program), 64u);

    assert(cpu.state == CPU_HALTED);
    assert(cpu.eax == 100u);
    assert(cpu.ebx == 200u);
    memory_free(&memory);
}

// 测试 TEST 指令
static void test_test_instruction(void) {
    memory_t memory;
    cpu_t cpu;
    // EAX = 5; EBX = 3
    // TEST EAX, EBX -> EAX & EBX = 1 (非零, ZF=0)
    // JZ should not jump, ECX = 1
    uint8_t program[] = {
        OP_MOV_REG_IMM, REG_EAX, 0x05, 0x00, 0x00, 0x00,
        OP_MOV_REG_IMM, REG_EBX, 0x03, 0x00, 0x00, 0x00,
        OP_TEST_REG_REG, REG_EAX, REG_EBX,
        OP_JZ_REL32, 0x06, 0x00, 0x00, 0x00,      // 跳转到 MOV ECX, 0
        OP_MOV_REG_IMM, REG_ECX, 0x01, 0x00, 0x00, 0x00,  // 不跳转时执行
        OP_JMP_REL32, 0x06, 0x00, 0x00, 0x00,     // 跳过 MOV ECX, 0
        OP_MOV_REG_IMM, REG_ECX, 0x00, 0x00, 0x00, 0x00,  // 跳转时执行
        OP_HLT
    };

    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    run_program(&cpu, &memory, program, sizeof(program), 64u);

    assert(cpu.state == CPU_HALTED);
    assert(cpu.ecx == 1u);  // 不跳转，ECX = 1
    assert((cpu.eflags & FLAG_ZF) == 0u);  // ZF=0
    memory_free(&memory);
}

// 测试移位指令
static void test_shift_operations(void) {
    memory_t memory;
    cpu_t cpu;
    // EAX = 1; EBX = 4
    // SHL EAX, EBX -> EAX = 16
    // MOV ECX, 32; SHR ECX, EBX -> ECX = 2
    uint8_t program[] = {
        OP_MOV_REG_IMM, REG_EAX, 0x01, 0x00, 0x00, 0x00,
        OP_MOV_REG_IMM, REG_EBX, 0x04, 0x00, 0x00, 0x00,
        OP_SHL_REG_REG, REG_EAX, REG_EBX,     // EAX = 16
        OP_MOV_REG_IMM, REG_ECX, 32, 0x00, 0x00, 0x00,
        OP_SHR_REG_REG, REG_ECX, REG_EBX,     // ECX = 2
        OP_HLT
    };

    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    run_program(&cpu, &memory, program, sizeof(program), 64u);

    assert(cpu.state == CPU_HALTED);
    assert(cpu.eax == 16u);   // 1 << 4 = 16
    assert(cpu.ecx == 2u);    // 32 >> 4 = 2
    memory_free(&memory);
}

// 测试标志位 CF 和 SF
static void test_flags_cf_sf(void) {
    memory_t memory;
    cpu_t cpu;
    
    // 测试 CF: 0xFFFFFFFF + 1 -> CF=1, ZF=1
    uint8_t program_cf[] = {
        OP_MOV_REG_IMM, REG_EAX, 0xFF, 0xFF, 0xFF, 0xFF,
        OP_MOV_REG_IMM, REG_EBX, 0x01, 0x00, 0x00, 0x00,
        OP_ADD_REG_REG, REG_EAX, REG_EBX,     // EAX = 0, CF=1, ZF=1
        OP_HLT
    };

    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    run_program(&cpu, &memory, program_cf, sizeof(program_cf), 64u);

    assert(cpu.state == CPU_HALTED);
    assert(cpu.eax == 0u);
    assert((cpu.eflags & FLAG_CF) != 0u);   // CF=1
    assert((cpu.eflags & FLAG_ZF) != 0u);   // ZF=1
    memory_free(&memory);
    
    // 测试 SF: 0x7FFFFFFF + 1 -> SF=1 (负数)
    uint8_t program_sf[] = {
        OP_MOV_REG_IMM, REG_ECX, 0xFF, 0xFF, 0xFF, 0x7F,
        OP_ADD_REG_IMM, REG_ECX, 0x01, 0x00, 0x00, 0x00, // ECX = 0x80000000, SF=1
        OP_HLT
    };

    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    run_program(&cpu, &memory, program_sf, sizeof(program_sf), 64u);

    assert(cpu.state == CPU_HALTED);
    assert(cpu.ecx == 0x80000000u);
    assert((cpu.eflags & FLAG_SF) != 0u);   // SF=1 (negative result)
    memory_free(&memory);
}

// 测试内存访问指令
static void test_memory_access(void) {
    memory_t memory;
    cpu_t cpu;
    // MOV EAX, 100
    // MOV EBX, 0x1000  ; 内存地址
    // MOV [EBX], EAX   ; 将 EAX (100) 存入地址 0x1000
    // MOV ECX, 0       ; 清零 ECX
    // MOV ECX, [EBX]   ; 从地址 0x1000 读取到 ECX
    // HLT
    uint8_t program[] = {
        OP_MOV_REG_IMM, REG_EAX, 100, 0x00, 0x00, 0x00,
        OP_MOV_REG_IMM, REG_EBX, 0x00, 0x10, 0x00, 0x00,  // EBX = 0x1000
        OP_MOV_MEM_REG, REG_EBX, REG_EAX,                  // [0x1000] = 100
        OP_MOV_REG_IMM, REG_ECX, 0x00, 0x00, 0x00, 0x00,
        OP_MOV_REG_MEM, REG_ECX, REG_EBX,                  // ECX = [0x1000]
        OP_HLT
    };

    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    run_program(&cpu, &memory, program, sizeof(program), 64u);

    assert(cpu.state == CPU_HALTED);
    assert(cpu.eax == 100u);
    assert(cpu.ecx == 100u);  // 从内存读取的值
    
    // 直接验证内存中的值
    uint8_t mem_val;
    memory_read8(&memory, 0x1000, &mem_val);
    assert(mem_val == 100u);
    
    memory_free(&memory);
}

// 测试 JC/JNC 指令
static void test_jc_jnc(void) {
    memory_t memory;
    cpu_t cpu;
    
    // 测试 JC 跳转 (CF=1 时跳转)
    // 0xFFFFFFFF + 1 -> CF=1
    uint8_t program_jc[] = {
        OP_MOV_REG_IMM, REG_EAX, 0xFF, 0xFF, 0xFF, 0xFF,
        OP_ADD_REG_IMM, REG_EAX, 0x01, 0x00, 0x00, 0x00,  // CF=1
        OP_JC_REL32, 0x06, 0x00, 0x00, 0x00,              // 跳转
        OP_MOV_REG_IMM, REG_EBX, 0x00, 0x00, 0x00, 0x00,  // 跳过
        OP_MOV_REG_IMM, REG_EBX, 0x01, 0x00, 0x00, 0x00,  // 执行
        OP_HLT
    };

    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    run_program(&cpu, &memory, program_jc, sizeof(program_jc), 64u);
    assert(cpu.state == CPU_HALTED);
    assert(cpu.ebx == 1u);  // JC 跳转成功
    memory_free(&memory);
    
    // 测试 JNC 跳转 (CF=0 时跳转)
    uint8_t program_jnc[] = {
        OP_MOV_REG_IMM, REG_EAX, 0x05, 0x00, 0x00, 0x00,
        OP_ADD_REG_IMM, REG_EAX, 0x01, 0x00, 0x00, 0x00,  // CF=0
        OP_JNC_REL32, 0x06, 0x00, 0x00, 0x00,            // 跳转
        OP_MOV_REG_IMM, REG_EBX, 0x00, 0x00, 0x00, 0x00,  // 跳过
        OP_MOV_REG_IMM, REG_EBX, 0x02, 0x00, 0x00, 0x00,  // 执行
        OP_HLT
    };

    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    run_program(&cpu, &memory, program_jnc, sizeof(program_jnc), 64u);
    assert(cpu.state == CPU_HALTED);
    assert(cpu.ebx == 2u);  // JNC 跳转成功
    memory_free(&memory);
}

// 测试 JS/JNS 指令
static void test_js_jns(void) {
    memory_t memory;
    cpu_t cpu;
    
    // 测试 JS 跳转 (SF=1 时跳转，结果为负)
    // 0x7FFFFFFF + 1 = 0x80000000 -> SF=1
    uint8_t program_js[] = {
        OP_MOV_REG_IMM, REG_EAX, 0xFF, 0xFF, 0xFF, 0x7F,
        OP_ADD_REG_IMM, REG_EAX, 0x01, 0x00, 0x00, 0x00,  // SF=1
        OP_JS_REL32, 0x06, 0x00, 0x00, 0x00,              // 跳转
        OP_MOV_REG_IMM, REG_EBX, 0x00, 0x00, 0x00, 0x00,  // 跳过
        OP_MOV_REG_IMM, REG_EBX, 0x01, 0x00, 0x00, 0x00,  // 执行
        OP_HLT
    };

    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    run_program(&cpu, &memory, program_js, sizeof(program_js), 64u);
    assert(cpu.state == CPU_HALTED);
    assert(cpu.ebx == 1u);  // JS 跳转成功
    memory_free(&memory);
}

// 测试 JO/JNO 指令
static void test_jo_jno(void) {
    memory_t memory;
    cpu_t cpu;
    
    // 测试 JO 跳转 (OF=1 时跳转，有符号溢出)
    // 0x7FFFFFFF + 1 -> OF=1 (正数变负数)
    uint8_t program_jo[] = {
        OP_MOV_REG_IMM, REG_EAX, 0xFF, 0xFF, 0xFF, 0x7F,
        OP_ADD_REG_IMM, REG_EAX, 0x01, 0x00, 0x00, 0x00,  // OF=1
        OP_JO_REL32, 0x06, 0x00, 0x00, 0x00,              // 跳转
        OP_MOV_REG_IMM, REG_EBX, 0x00, 0x00, 0x00, 0x00,  // 跳过
        OP_MOV_REG_IMM, REG_EBX, 0x01, 0x00, 0x00, 0x00,  // 执行
        OP_HLT
    };

    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    run_program(&cpu, &memory, program_jo, sizeof(program_jo), 64u);
    assert(cpu.state == CPU_HALTED);
    assert(cpu.ebx == 1u);  // JO 跳转成功
    memory_free(&memory);
}

int main(void) {
    test_cmp_jz_taken();
    test_stack_add_sub_jnz();
    test_jmp_skip();
    test_logical_operations();
    test_not_inc_dec();
    test_imm_arithmetic();
    test_call_ret();
    test_test_instruction();
    test_shift_operations();
    test_flags_cf_sf();
    test_memory_access();
    test_jc_jnc();
    test_js_jns();
    test_jo_jno();
    return 0;
}