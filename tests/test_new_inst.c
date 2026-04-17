#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mycpu/cpu.h"
#include "mycpu/memory.h"
#include "mycpu/isa.h"

// Test new instructions: NEG, NOP, CLC, STC, LOOP

int main(void) {
    memory_t memory;
    cpu_t cpu;
    int passed = 0;
    int total = 0;
    
    if (!memory_init(&memory, MYCPU_MEM_SIZE)) {
        fprintf(stderr, "Memory init failed\n");
        return 1;
    }
    
    printf("Testing new instructions\n");
    printf("========================\n\n");
    
    // Test 1: NEG instruction
    printf("Test 1: NEG instruction\n");
    {
        uint8_t program[] = {
            OP_MOV_REG_IMM, REG_EAX, 100, 0, 0, 0,  // EAX = 100
            OP_NEG_REG, REG_EAX,                     // EAX = -100
            OP_HLT
        };
        for (size_t i = 0; i < sizeof(program); i++) {
            memory_write8(&memory, (uint32_t)i, program[i]);
        }
        cpu_init(&cpu);
        cpu_reset(&cpu, 0, (uint32_t)memory.size);
        while (cpu.state == CPU_RUNNING) cpu_step(&cpu, &memory);
        
        int32_t result = (int32_t)cpu.eax;
        total++;
        if (result == -100) {
            printf("  PASS: NEG 100 -> %d\n", result);
            passed++;
        } else {
            printf("  FAIL: NEG 100 -> %d (expected -100)\n", result);
        }
    }
    
    // Test 2: NOP instruction
    printf("\nTest 2: NOP instruction\n");
    {
        uint8_t program[] = {
            OP_MOV_REG_IMM, REG_EAX, 42, 0, 0, 0,
            OP_NOP,
            OP_NOP,
            OP_HLT
        };
        for (size_t i = 0; i < sizeof(program); i++) {
            memory_write8(&memory, (uint32_t)i, program[i]);
        }
        cpu_init(&cpu);
        cpu_reset(&cpu, 0, (uint32_t)memory.size);
        int steps = 0;
        while (cpu.state == CPU_RUNNING && steps < 10) {
            cpu_step(&cpu, &memory);
            steps++;
        }
        
        total++;
        if (cpu.eax == 42 && cpu.state == CPU_HALTED) {
            printf("  PASS: NOP works, EAX = %u, steps = %d\n", cpu.eax, steps);
            passed++;
        } else {
            printf("  FAIL: EAX = %u, state = %d\n", cpu.eax, cpu.state);
        }
    }
    
    // Test 3: CLC and STC instructions
    printf("\nTest 3: CLC and STC instructions\n");
    {
        uint8_t program[] = {
            OP_STC,        // CF = 1
            OP_CLC,        // CF = 0
            OP_STC,        // CF = 1
            OP_HLT
        };
        for (size_t i = 0; i < sizeof(program); i++) {
            memory_write8(&memory, (uint32_t)i, program[i]);
        }
        cpu_init(&cpu);
        cpu_reset(&cpu, 0, (uint32_t)memory.size);
        while (cpu.state == CPU_RUNNING) cpu_step(&cpu, &memory);
        
        total++;
        if ((cpu.eflags & FLAG_CF) != 0) {
            printf("  PASS: CF = 1 after STC\n");
            passed++;
        } else {
            printf("  FAIL: CF = 0 (expected 1)\n");
        }
    }
    
    // Test 4: LOOP instruction
    printf("\nTest 4: LOOP instruction\n");
    {
        // Program: loop 5 times, each iteration increments EAX
        // ECX starts at 5, each LOOP decrements ECX
        // Layout:
        // 0x00-0x05: MOV EAX, 0
        // 0x06-0x0B: MOV ECX, 5
        // 0x0C-0x0D: INC EAX        <- loop_start
        // 0x0E-0x0F: LOOP -4        -> target = 0x0E + 2 + (-4) = 0x0C
        // 0x10:      HLT
        uint8_t program[] = {
            OP_MOV_REG_IMM, REG_EAX, 0, 0, 0, 0,    // EAX = 0
            OP_MOV_REG_IMM, REG_ECX, 5, 0, 0, 0,    // ECX = 5
            // loop_start (address 0x0C):
            OP_INC_REG, REG_EAX,                     // EAX++
            OP_LOOP_REL8, (uint8_t)(-4 & 0xFF),      // LOOP back to INC
            OP_HLT
        };
        for (size_t i = 0; i < sizeof(program); i++) {
            memory_write8(&memory, (uint32_t)i, program[i]);
        }
        cpu_init(&cpu);
        cpu_reset(&cpu, 0, (uint32_t)memory.size);
        while (cpu.state == CPU_RUNNING) cpu_step(&cpu, &memory);
        
        total++;
        if (cpu.eax == 5 && cpu.ecx == 0) {
            printf("  PASS: LOOP executed 5 times, EAX = %u, ECX = %u\n", cpu.eax, cpu.ecx);
            passed++;
        } else {
            printf("  FAIL: EAX = %u (expected 5), ECX = %u (expected 0)\n", cpu.eax, cpu.ecx);
        }
    }
    
    // Test 5: NEG with zero
    printf("\nTest 5: NEG zero\n");
    {
        uint8_t program[] = {
            OP_MOV_REG_IMM, REG_EAX, 0, 0, 0, 0,    // EAX = 0
            OP_NEG_REG, REG_EAX,                     // EAX = -0 = 0, CF should be 0
            OP_HLT
        };
        for (size_t i = 0; i < sizeof(program); i++) {
            memory_write8(&memory, (uint32_t)i, program[i]);
        }
        cpu_init(&cpu);
        cpu_reset(&cpu, 0, (uint32_t)memory.size);
        while (cpu.state == CPU_RUNNING) cpu_step(&cpu, &memory);
        
        total++;
        bool cf_correct = (cpu.eflags & FLAG_CF) == 0;
        bool zf_correct = (cpu.eflags & FLAG_ZF) != 0;
        if (cpu.eax == 0 && cf_correct && zf_correct) {
            printf("  PASS: NEG 0 -> 0, CF=0, ZF=1\n");
            passed++;
        } else {
            printf("  FAIL: EAX=%u, CF=%d, ZF=%d\n", cpu.eax, 
                   (cpu.eflags & FLAG_CF) ? 1 : 0,
                   (cpu.eflags & FLAG_ZF) ? 1 : 0);
        }
    }
    
    printf("\n========================\n");
    printf("Results: %d/%d tests passed\n", passed, total);
    
    memory_free(&memory);
    return (passed == total) ? 0 : 1;
}
