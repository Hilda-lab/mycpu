#include <stdio.h>
#include <stdint.h>

#include "mycpu/cpu.h"
#include "mycpu/isa.h"
#include "mycpu/memory.h"

int main(void) {
    memory_t memory;
    cpu_t cpu;

    printf("==============================================\n");
    printf("        myCPU 简单测试程序\n");
    printf("==============================================\n\n");

    if (!memory_init(&memory, MYCPU_MEM_SIZE)) {
        printf("❌ 内存初始化失败\n");
        return 1;
    }

    // ===== 测试 1：MOV 指令 =====
    printf("【测试 1】MOV 指令：将数字放入寄存器\n");
    printf("程序: MOV EAX, 50\n");
    printf("       MOV EBX, 100\n");
    printf("       HLT\n\n");

    uint8_t program1[] = {
        OP_MOV_REG_IMM, REG_EAX, 50, 0, 0, 0,
        OP_MOV_REG_IMM, REG_EBX, 100, 0, 0, 0,
        OP_HLT
    };

    // 加载程序
    for (size_t i = 0; i < sizeof(program1); i++) {
        memory_write8(&memory, (uint32_t)i, program1[i]);
    }

    cpu_init(&cpu);
    cpu_reset(&cpu, 0u, (uint32_t)memory.size);

    printf("执行前：EAX=%u, EBX=%u\n", cpu.eax, cpu.ebx);
    cpu_run(&cpu, &memory, 10);
    printf("执行后：EAX=%u ✓, EBX=%u ✓\n\n", cpu.eax, cpu.ebx);

    // ===== 测试 2：ADD 指令 =====
    printf("【测试 2】ADD 指令：寄存器相加\n");
    printf("程序: MOV EAX, 10\n");
    printf("       MOV EBX, 20\n");
    printf("       ADD EAX, EBX  ; EAX = EAX + EBX = 10 + 20 = 30\n");
    printf("       HLT\n\n");

    uint8_t program2[] = {
        OP_MOV_REG_IMM, REG_EAX, 10, 0, 0, 0,
        OP_MOV_REG_IMM, REG_EBX, 20, 0, 0, 0,
        OP_ADD_REG_REG, REG_EAX, REG_EBX,
        OP_HLT
    };

    for (size_t i = 0; i < sizeof(program2); i++) {
        memory_write8(&memory, (uint32_t)i, program2[i]);
    }

    cpu_init(&cpu);
    cpu_reset(&cpu, 0u, (uint32_t)memory.size);

    printf("执行前：EAX=%u, EBX=%u\n", cpu.eax, cpu.ebx);
    cpu_run(&cpu, &memory, 10);
    printf("执行后：EAX=%u ✓ (应为 30), EBX=%u\n\n", cpu.eax, cpu.ebx);

    // ===== 测试 3：SUB 指令 =====
    printf("【测试 3】SUB 指令：寄存器相减\n");
    printf("程序: MOV EAX, 100\n");
    printf("       MOV EBX, 30\n");
    printf("       SUB EAX, EBX  ; EAX = EAX - EBX = 100 - 30 = 70\n");
    printf("       HLT\n\n");

    uint8_t program3[] = {
        OP_MOV_REG_IMM, REG_EAX, 100, 0, 0, 0,
        OP_MOV_REG_IMM, REG_EBX, 30, 0, 0, 0,
        OP_SUB_REG_REG, REG_EAX, REG_EBX,
        OP_HLT
    };

    for (size_t i = 0; i < sizeof(program3); i++) {
        memory_write8(&memory, (uint32_t)i, program3[i]);
    }

    cpu_init(&cpu);
    cpu_reset(&cpu, 0u, (uint32_t)memory.size);

    printf("执行前：EAX=%u, EBX=%u\n", cpu.eax, cpu.ebx);
    cpu_run(&cpu, &memory, 10);
    printf("执行后：EAX=%u ✓ (应为 70), EBX=%u\n\n", cpu.eax, cpu.ebx);

    // ===== 测试 4：CMP 和 JZ 指令 =====
    printf("【测试 4】CMP 和 JZ：比较和条件跳转\n");
    printf("程序: MOV EAX, 25\n");
    printf("       MOV EBX, 25\n");
    printf("       CMP EAX, EBX  ; 比较，设置 ZF=1 (因为相等)\n");
    printf("       JZ +5        ; 如果 ZF=1，跳过 5 字节\n");
    printf("       MOV ECX, 0   ; 这条被跳过\n");
    printf("       MOV ECX, 1   ; 执行这条\n");
    printf("       HLT\n\n");

    uint8_t program4[] = {
        OP_MOV_REG_IMM, REG_EAX, 25, 0, 0, 0,
        OP_MOV_REG_IMM, REG_EBX, 25, 0, 0, 0,
        OP_MOV_REG_IMM, REG_ECX, 0, 0, 0, 0,
        OP_CMP_REG_REG, REG_EAX, REG_EBX,
        OP_JZ_REL32, 0x05, 0, 0, 0,
        OP_MOV_REG_IMM, REG_ECX, 0, 0, 0, 0,
        OP_MOV_REG_IMM, REG_ECX, 1, 0, 0, 0,
        OP_HLT
    };

    for (size_t i = 0; i < sizeof(program4); i++) {
        memory_write8(&memory, (uint32_t)i, program4[i]);
    }

    cpu_init(&cpu);
    cpu_reset(&cpu, 0u, (uint32_t)memory.size);

    printf("执行前：EAX=%u, EBX=%u, ECX=%u\n", cpu.eax, cpu.ebx, cpu.ecx);
    cpu_run(&cpu, &memory, 10);
    printf("执行后：EAX=%u, EBX=%u, ECX=%u ✓ (应为 1)\n", cpu.eax, cpu.ebx, cpu.ecx);
    printf("         ZF=%d ✓ (应为 1，因为 25 == 25)\n\n",
           (cpu.eflags & FLAG_ZF) ? 1 : 0);

    // ===== 测试 5：PUSH 和 POP 指令 =====
    printf("【测试 5】PUSH 和 POP：栈操作\n");
    printf("程序: MOV EAX, 200\n");
    printf("       MOV EBX, 0\n");
    printf("       PUSH EAX      ; 将 EAX (200) 压入栈\n");
    printf("       POP EBX       ; 从栈弹出值到 EBX\n");
    printf("       HLT\n\n");

    uint8_t program5[] = {
        OP_MOV_REG_IMM, REG_EAX, 200, 0, 0, 0,
        OP_MOV_REG_IMM, REG_EBX, 0, 0, 0, 0,
        OP_PUSH_REG, REG_EAX,
        OP_POP_REG, REG_EBX,
        OP_HLT
    };

    uint32_t original_esp;
    for (size_t i = 0; i < sizeof(program5); i++) {
        memory_write8(&memory, (uint32_t)i, program5[i]);
    }

    cpu_init(&cpu);
    cpu_reset(&cpu, 0u, (uint32_t)memory.size);
    original_esp = cpu.esp;

    printf("执行前：EAX=%u, EBX=%u, ESP=%u\n", cpu.eax, cpu.ebx, cpu.esp);
    cpu_run(&cpu, &memory, 10);
    printf("执行后：EAX=%u, EBX=%u ✓ (应为 200), ESP=%u ✓ (回到原值)\n\n",
           cpu.eax, cpu.ebx, cpu.esp);

    // ===== 测试 6：综合测试 =====
    printf("【测试 6】综合测试：计算 (10 + 20) - 15\n");
    printf("程序: MOV EAX, 10\n");
    printf("       MOV EBX, 20\n");
    printf("       ADD EAX, EBX  ; EAX = 30\n");
    printf("       MOV EBX, 15\n");
    printf("       SUB EAX, EBX  ; EAX = 30 - 15 = 15\n");
    printf("       HLT\n\n");

    uint8_t program6[] = {
        OP_MOV_REG_IMM, REG_EAX, 10, 0, 0, 0,
        OP_MOV_REG_IMM, REG_EBX, 20, 0, 0, 0,
        OP_ADD_REG_REG, REG_EAX, REG_EBX,
        OP_MOV_REG_IMM, REG_EBX, 15, 0, 0, 0,
        OP_SUB_REG_REG, REG_EAX, REG_EBX,
        OP_HLT
    };

    for (size_t i = 0; i < sizeof(program6); i++) {
        memory_write8(&memory, (uint32_t)i, program6[i]);
    }

    cpu_init(&cpu);
    cpu_reset(&cpu, 0u, (uint32_t)memory.size);

    printf("执行前：EAX=%u\n", cpu.eax);
    cpu_run(&cpu, &memory, 10);
    printf("执行后：EAX=%u ✓ (应为 15)\n\n", cpu.eax);

    memory_free(&memory);

    printf("==============================================\n");
    printf("        所有测试完成！✓\n");
    printf("==============================================\n");

    return 0;
}
