#include <stdio.h>

#include "mycpu/cpu.h"
#include "mycpu/isa.h"
#include "mycpu/memory.h"
#include "mycpu/uart.h"

int main(void) {
    memory_t memory;
    cpu_t cpu;

    uint8_t program[] = {
        OP_MOV_REG_IMM, REG_EAX, 0x2A, 0x00, 0x00, 0x00,
        OP_HLT
    };

    if (!memory_init(&memory, MYCPU_MEM_SIZE)) {
        fprintf(stderr, "memory_init failed\n");
        return 1;
    }

    for (size_t i = 0; i < sizeof(program); i++) {
        if (!memory_write8(&memory, (uint32_t)i, program[i])) {
            fprintf(stderr, "program load failed at %zu\n", i);
            memory_free(&memory);
            return 1;
        }
    }

    cpu_init(&cpu);
    cpu_reset(&cpu, 0u, (uint32_t)memory.size);
    cpu_run(&cpu, &memory, 64u);

    uart_putc('O');
    uart_putc('K');
    uart_putc('\n');

    printf("myCPU demo done, EAX=%u EIP=0x%08X state=%d\n", cpu.eax, cpu.eip, cpu.state);
    memory_free(&memory);
    return 0;
}
