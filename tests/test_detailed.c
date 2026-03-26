#include <stdio.h>
#include <stdint.h>

#include "mycpu/cpu.h"
#include "mycpu/isa.h"
#include "mycpu/memory.h"

// 辅助函数：打印 CPU 状态
void print_cpu_state(const cpu_t *cpu, const char *step_name) {
    printf("\n[%s]\n", step_name);
    printf("  EAX=%-6u EBX=%-6u ECX=%-6u EDX=%-6u\n", cpu->eax, cpu->ebx, cpu->ecx, cpu->edx);
    printf("  ESI=%-6u EDI=%-6u EBP=%-6u ESP=%-6u\n", cpu->esi, cpu->edi, cpu->ebp, cpu->esp);
    printf("  EIP=0x%08X  EFLAGS=0x%08X\n", cpu->eip, cpu->eflags);
    printf("  ZF=%d (结果为零标志)\n", (cpu->eflags & FLAG_ZF) ? 1 : 0);
}

// 辅助函数：打印指令
void print_instruction(const uint8_t *program, uint32_t eip) {
    printf("  当前指令: ");
    uint8_t opcode = program[eip];

    switch (opcode) {
        case OP_MOV_REG_IMM:
            printf("MOV R%d, %u\n", program[eip+1],
                   *(uint32_t*)&program[eip+2]);
            break;
        case OP_MOV_REG_REG:
            printf("MOV R%d, R%d\n", program[eip+1], program[eip+2]);
            break;
        case OP_ADD_REG_REG:
            printf("ADD R%d, R%d\n", program[eip+1], program[eip+2]);
            break;
        case OP_SUB_REG_REG:
            printf("SUB R%d, R%d\n", program[eip+1], program[eip+2]);
            break;
        case OP_CMP_REG_REG:
            printf("CMP R%d, R%d\n", program[eip+1], program[eip+2]);
            break;
        case OP_JMP_REL32:
            printf("JMP %+d\n", *(int32_t*)&program[eip+1]);
            break;
        case OP_JZ_REL32:
            printf("JZ %+d\n", *(int32_t*)&program[eip+1]);
            break;
        case OP_JNZ_REL32:
            printf("JNZ %+d\n", *(int32_t*)&program[eip+1]);
            break;
        case OP_PUSH_REG:
            printf("PUSH R%d\n", program[eip+1]);
            break;
        case OP_POP_REG:
            printf("POP R%d\n", program[eip+1]);
            break;
        case OP_HLT:
            printf("HLT (停止)\n");
            break;
        default:
            printf("未知指令: 0x%02X\n", opcode);
    }
}

int main(void) {
    memory_t memory;
    cpu_t cpu;

    printf("========================================\n");
    printf("  myCPU 详细执行过程演示\n");
    printf("========================================\n");

    // 测试程序 1：简单的算术运算
    printf("\n【测试 1：算术运算】\n");
    printf("目的：测试 MOV、ADD、SUB 指令\n");
    printf("程序：EAX=10, EBX=20, ECX=EAX+EBX, EDX=ECX-EAX\n\n");

    uint8_t program1[] = {
        OP_MOV_REG_IMM, REG_EAX, 10, 0, 0, 0,    // MOV EAX, 10
        OP_MOV_REG_IMM, REG_EBX, 20, 0, 0, 0,    // MOV EBX, 20
        OP_MOV_REG_IMM, REG_ECX, 0, 0, 0, 0,     // MOV ECX, 0
        OP_MOV_REG_IMM, REG_EDX, 0, 0, 0, 0,     // MOV EDX, 0
        OP_ADD_REG_REG, REG_ECX, REG_EAX,        // ADD ECX, EAX
        OP_ADD_REG_REG, REG_ECX, REG_EBX,        // ADD ECX, EBX
        OP_SUB_REG_REG, REG_EDX, REG_ECX,        // SUB EDX, ECX
        OP_SUB_REG_REG, REG_EDX, REG_EAX,        // SUB EDX, EAX
        OP_HLT
    };

    if (!memory_init(&memory, MYCPU_MEM_SIZE)) {
        printf("❌ 内存初始化失败\n");
        return 1;
    }

    // 加载程序到内存
    for (size_t i = 0; i < sizeof(program1); i++) {
        memory_write8(&memory, (uint32_t)i, program1[i]);
    }

    // 初始化 CPU
    cpu_init(&cpu);
    cpu_reset(&cpu, 0u, (uint32_t)memory.size);

    print_cpu_state(&cpu, "初始状态");

    // 逐步执行指令
    int step = 0;
    while (cpu.state == CPU_RUNNING && step < 20) {
        print_instruction(program1, cpu.eip);
        cpu_step(&cpu, &memory);
        print_cpu_state(&cpu, "执行后");
        step++;
    }

    printf("\n✓ 测试结果：\n");
    printf("  EAX=%u (应为 10)\n", cpu.eax);
    printf("  EBX=%u (应为 20)\n", cpu.ebx);
    printf("  ECX=%u (应为 30 = 10+20)\n", cpu.ecx);
    printf("  EDX=%u (应为 20 = 30-10)\n", cpu.edx);
    printf("  状态: %s\n",
           cpu.state == CPU_HALTED ? "正常停止 ✓" : "异常 ✗");

    // 测试程序 2：栈操作
    printf("\n========================================\n");
    printf("\n【测试 2：栈操作】\n");
    printf("目的：测试 PUSH、POP 指令\n");
    printf("程序：将 EAX 压栈，再弹出到 EBX\n\n");

    uint8_t program2[] = {
        OP_MOV_REG_IMM, REG_EAX, 100, 0, 0, 0,   // MOV EAX, 100
        OP_MOV_REG_IMM, REG_EBX, 0, 0, 0, 0,     // MOV EBX, 0
        OP_PUSH_REG, REG_EAX,                     // PUSH EAX (将 100 压入栈)
        OP_POP_REG, REG_EBX,                      // POP EBX (从栈弹出值到 EBX)
        OP_HLT
    };

    // 重新初始化
    cpu_init(&cpu);
    cpu_reset(&cpu, 0u, (uint32_t)memory.size);

    for (size_t i = 0; i < sizeof(program2); i++) {
        memory_write8(&memory, (uint32_t)i, program2[i]);
    }

    uint32_t original_esp = cpu.esp;
    print_cpu_state(&cpu, "初始状态");

    step = 0;
    while (cpu.state == CPU_RUNNING && step < 20) {
        print_instruction(program2, cpu.eip);
        cpu_step(&cpu, &memory);
        print_cpu_state(&cpu, "执行后");
        step++;
    }

    printf("\n✓ 测试结果：\n");
    printf("  EAX=%u (应为 100)\n", cpu.eax);
    printf("  EBX=%u (应为 100，从栈中弹出)\n", cpu.ebx);
    printf("  ESP=%u (应回到初始值 %u)\n", cpu.esp, original_esp);
    printf("  状态: %s\n",
           cpu.state == CPU_HALTED ? "正常停止 ✓" : "异常 ✗");

    // 测试程序 3：条件跳转
    printf("\n========================================\n");
    printf("\n【测试 3：条件跳转】\n");
    printf("目的：测试 CMP、JZ、JNZ 指令\n");
    printf("程序：比较两个数，根据结果跳转\n\n");

    uint8_t program3[] = {
        OP_MOV_REG_IMM, REG_EAX, 15, 0, 0, 0,   // MOV EAX, 15
        OP_MOV_REG_IMM, REG_EBX, 15, 0, 0, 0,   // MOV EBX, 15
        OP_MOV_REG_IMM, REG_ECX, 0, 0, 0, 0,     // MOV ECX, 0
        OP_CMP_REG_REG, REG_EAX, REG_EBX,        // CMP EAX, EBX (15 == 15)
        OP_JZ_REL32, 0x05, 0, 0, 0,              // JZ +5 (相等时跳转)
        OP_MOV_REG_IMM, REG_ECX, 1, 0, 0, 0,     // 这条被跳过
        OP_MOV_REG_IMM, REG_ECX, 2, 0, 0, 0,     // 执行这条：ECX = 2
        OP_HLT
    };

    cpu_init(&cpu);
    cpu_reset(&cpu, 0u, (uint32_t)memory.size);

    for (size_t i = 0; i < sizeof(program3); i++) {
        memory_write8(&memory, (uint32_t)i, program3[i]);
    }

    print_cpu_state(&cpu, "初始状态");

    step = 0;
    while (cpu.state == CPU_RUNNING && step < 20) {
        print_instruction(program3, cpu.eip);
        cpu_step(&cpu, &memory);
        print_cpu_state(&cpu, "执行后");
        step++;
    }

    printf("\n✓ 测试结果：\n");
    printf("  ECX=%u (应为 2，因为 JZ 跳转成功)\n", cpu.ecx);
    printf("  ZF=%d (应为 1，因为 EAX == EBX)\n",
           (cpu.eflags & FLAG_ZF) ? 1 : 0);
    printf("  状态: %s\n",
           cpu.state == CPU_HALTED ? "正常停止 ✓" : "异常 ✗");

    memory_free(&memory);

    printf("\n========================================\n");
    printf("  所有测试完成！\n");
    printf("========================================\n");

    return 0;
}
