#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mycpu/cpu.h"
#include "mycpu/memory.h"
#include "mycpu/debugger.h"
#include "mycpu/isa.h"

// Demo program: Calculate Fibonacci sequence
static const uint8_t demo_program[] = {
    // Initialize
    OP_MOV_REG_IMM, REG_EAX, 0x01, 0x00, 0x00, 0x00,  // EAX = 1 (fib(n-1))
    OP_MOV_REG_IMM, REG_EBX, 0x01, 0x00, 0x00, 0x00,  // EBX = 1 (fib(n-2))
    OP_MOV_REG_IMM, REG_ECX, 0x0A, 0x00, 0x00, 0x00,  // ECX = 10 (counter)
    
    // Loop start (address 0x12)
    OP_CMP_REG_IMM, REG_ECX, 0x00, 0x00, 0x00, 0x00,  // CMP ECX, 0
    OP_JZ_REL32, 0x13, 0x00, 0x00, 0x00,              // JZ to HLT (rel=19)
    
    // Calculate next Fibonacci number
    OP_MOV_REG_REG, REG_EDX, REG_EAX,                 // EDX = EAX
    OP_ADD_REG_REG, REG_EDX, REG_EBX,                 // EDX = EAX + EBX
    OP_MOV_REG_REG, REG_EBX, REG_EAX,                 // EBX = EAX
    OP_MOV_REG_REG, REG_EAX, REG_EDX,                 // EAX = EDX
    
    // Decrement counter
    OP_DEC_REG, REG_ECX,                              // ECX--
    
    // Jump back to loop start
    OP_JMP_REL32, 0xE2, 0xFF, 0xFF, 0xFF,             // JMP -30 (back to CMP at 0x12)
    
    // End (address 0x32)
    OP_HLT
};

int main(void) {
    memory_t memory;
    cpu_t cpu;
    debugger_t debugger;
    
    printf("myCPU Visual Debugger\n");
    printf("=====================\n\n");
    
    // Initialize memory
    if (!memory_init(&memory, MYCPU_MEM_SIZE)) {
        fprintf(stderr, "Memory initialization failed\n");
        return 1;
    }
    
    // Load program
    for (size_t i = 0; i < sizeof(demo_program); i++) {
        memory_write8(&memory, (uint32_t)i, demo_program[i]);
    }
    printf("Loaded demo program: Fibonacci sequence calculation\n");
    printf("Program size: %zu bytes\n", sizeof(demo_program));
    
    // Initialize CPU
    cpu_init(&cpu);
    cpu_reset(&cpu, 0, (uint32_t)memory.size);
    printf("CPU initialized\n");
    
    // Initialize debugger
    debugger_init(&debugger, &cpu, &memory);
    printf("Debugger initialized\n\n");
    
    printf("Starting visual interface...\n");
    printf("Press ENTER to continue...\n");
    getchar();
    
    // Launch debugger
    debugger_run(&debugger);
    
    // Print final result
    printf("\n========== RESULT ==========\n");
    printf("EAX = %u (11th Fibonacci number)\n", cpu.eax);
    printf("EBX = %u\n", cpu.ebx);
    printf("ECX = %u\n", cpu.ecx);
    printf("Steps executed: %u\n", debugger.step_count);
    printf("============================\n");
    
    memory_free(&memory);
    return 0;
}
