#include <stdio.h>

#include "mycpu/cpu.h"
#include "mycpu/uart.h"

int main(void) {
    cpu_t cpu;
    cpu_init(&cpu);
    cpu_reset(&cpu, 0x1000u);

    uart_putc('O');
    uart_putc('K');
    uart_putc('\n');

    printf("myCPU skeleton started, EIP=0x%08X\n", cpu.eip);
    return 0;
}
