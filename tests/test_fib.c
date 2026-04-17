#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mycpu/cpu.h"
#include "mycpu/memory.h"
#include "mycpu/isa.h"

// Demo program: Calculate Fibonacci sequence
static const uint8_t demo_program[] = {
    OP_MOV_REG_IMM, REG_EAX, 0x01, 0x00, 0x00, 0x00,  // EAX = 1
    OP_MOV_REG_IMM, REG_EBX, 0x01, 0x00, 0x00, 0x00,  // EBX = 1
    OP_MOV_REG_IMM, REG_ECX, 0x0A, 0x00, 0x00, 0x00,  // ECX = 10
    OP_CMP_REG_IMM, REG_ECX, 0x00, 0x00, 0x00, 0x00,  // CMP ECX, 0
    OP_JZ_REL32, 0x13, 0x00, 0x00, 0x00,              // JZ to HLT (rel=19)
    OP_MOV_REG_REG, REG_EDX, REG_EAX,                 // EDX = EAX
    OP_ADD_REG_REG, REG_EDX, REG_EBX,                 // EDX = EAX + EBX
    OP_MOV_REG_REG, REG_EBX, REG_EAX,                 // EBX = EAX
    OP_MOV_REG_REG, REG_EAX, REG_EDX,                 // EAX = EDX
    OP_DEC_REG, REG_ECX,                              // ECX--
    OP_JMP_REL32, 0xE2, 0xFF, 0xFF, 0xFF,             // JMP -30 (back to CMP)
    OP_HLT
};

int main(void) {
    memory_t memory;
    cpu_t cpu;
    
    if (!memory_init(&memory, MYCPU_MEM_SIZE)) {
        fprintf(stderr, "Memory init failed\n");
        return 1;
    }
    
    // Load program
    for (size_t i = 0; i < sizeof(demo_program); i++) {
        memory_write8(&memory, (uint32_t)i, demo_program[i]);
    }
    
    cpu_init(&cpu);
    cpu_reset(&cpu, 0, (uint32_t)memory.size);
    
    printf("Step-by-step execution:\n");
    printf("========================\n");
    
    int step = 0;
    while (cpu.state == CPU_RUNNING && step < 100) {
        printf("Step %2d: EIP=0x%08X EAX=%u EBX=%u ECX=%u EDX=%u\n",
               step, cpu.eip, cpu.eax, cpu.ebx, cpu.ecx, cpu.edx);
        cpu_step(&cpu, &memory);
        step++;
    }
    
    printf("\nFinal state:\n");
    printf("  CPU state: %s\n", cpu.state == CPU_HALTED ? "HALTED" : 
                               cpu.state == CPU_EXCEPTION ? "ERROR" : "UNKNOWN");
    printf("  EAX = %u (expected: 144)\n", cpu.eax);
    printf("  EBX = %u\n", cpu.ebx);
    printf("  ECX = %u\n", cpu.ecx);
    printf("  Steps: %d\n", step);
    
    memory_free(&memory);
    return (cpu.eax == 144) ? 0 : 1;
}
