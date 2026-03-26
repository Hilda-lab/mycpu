#include <stdio.h>

#include "mycpu/uart.h"

void uart_putc(uint8_t ch) {
    putchar((int)ch);
}
