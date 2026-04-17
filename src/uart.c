#include "mycpu/uart.h"

#include <string.h>
#include <stdio.h>

// 简单的UART输出函数（向后兼容）
void uart_putc(uint8_t ch) {
    putchar((int)ch);
}

// UART初始化
void uart_init(uart_t *uart, pic_t *pic, uint8_t irq) {
    memset(uart, 0, sizeof(uart_t));
    uart->pic = pic;
    uart->irq = irq;
    
    // 默认波特率除数（115200）
    uart->dll = 1;
    uart->dlm = 0;
    
    // 默认线路状态：发送器空闲，发送保持寄存器空
    uart->lsr = UART_LSR_TX_EMPTY | UART_LSR_TX_IDLE;
}

void uart_write_reg(uart_t *uart, uint8_t reg, uint8_t value) {
    bool dlab = (uart->lcr & 0x80) != 0;  // 除数锁存访问位
    
    if (dlab && reg <= 1) {
        // 访问除数锁存
        if (reg == 0) {
            uart->dll = value;
        } else {
            uart->dlm = value;
        }
        return;
    }
    
    switch (reg) {
        case UART_THR:  // 发送保持寄存器
            if (uart->tx_count < UART_BUFFER_SIZE) {
                uart->tx_buffer[uart->tx_head] = value;
                uart->tx_head = (uart->tx_head + 1) % UART_BUFFER_SIZE;
                uart->tx_count++;
                uart->lsr &= ~UART_LSR_TX_EMPTY;
                
                // 如果有发送回调，立即发送
                if (uart->tx_callback) {
                    uint8_t ch = value;
                    uart->tx_callback(ch);
                }
                
                // 发送后标记为空
                uart->tx_count--;
                uart->lsr |= UART_LSR_TX_EMPTY | UART_LSR_TX_IDLE;
                
                // 检查是否产生发送空中断
                if ((uart->ier & UART_IER_TX_ENABLE) && (uart->lsr & UART_LSR_TX_EMPTY)) {
                    uart_update_interrupt(uart);
                }
            }
            break;
            
        case UART_IER:  // 中断使能寄存器
            uart->ier = value & 0x0F;  // 只保留低4位
            uart_update_interrupt(uart);
            break;
            
        case UART_FCR:  // FIFO控制寄存器
            uart->fcr = value;
            if (value & UART_FCR_CLEAR_RX) {
                uart->rx_count = 0;
                uart->rx_head = 0;
                uart->rx_tail = 0;
            }
            if (value & UART_FCR_CLEAR_TX) {
                uart->tx_count = 0;
                uart->tx_head = 0;
                uart->tx_tail = 0;
            }
            break;
            
        case UART_LCR:  // 线路控制寄存器
            uart->lcr = value;
            break;
            
        case UART_MCR:  // Modem控制寄存器
            uart->mcr = value & 0x1F;
            break;
            
        case UART_SCR:  // 暂存寄存器
            uart->scr = value;
            break;
    }
}

uint8_t uart_read_reg(uart_t *uart, uint8_t reg) {
    bool dlab = (uart->lcr & 0x80) != 0;
    
    if (dlab && reg <= 1) {
        return (reg == 0) ? uart->dll : uart->dlm;
    }
    
    switch (reg) {
        case UART_RBR:  // 接收缓冲寄存器
            if (uart->rx_count > 0) {
                uint8_t value = uart->rx_buffer[uart->rx_tail];
                uart->rx_tail = (uart->rx_tail + 1) % UART_BUFFER_SIZE;
                uart->rx_count--;
                
                if (uart->rx_count == 0) {
                    uart->lsr &= ~UART_LSR_RX_READY;
                }
                
                return value;
            }
            return 0;
            
        case UART_IER:
            return uart->ier;
            
        case UART_IIR: {
            // 中断标识寄存器
            // 检查中断优先级
            uint8_t iir = 0x01;  // 无中断
            
            // 最高优先级：线路状态中断
            if ((uart->ier & UART_IER_LINE_STATUS) && (uart->lsr & 0x1E)) {
                iir = 0x06;  // 线路状态中断
            }
            // 次高优先级：接收数据中断
            else if ((uart->ier & UART_IER_RX_ENABLE) && (uart->lsr & UART_LSR_RX_READY)) {
                iir = 0x04;  // 接收数据可用中断
            }
            // 次高优先级：超时中断（FIFO模式下）
            else if ((uart->ier & UART_IER_RX_ENABLE) && uart->rx_count > 0) {
                iir = 0x0C;  // 字符超时中断
            }
            // 最低优先级：发送空中断
            else if ((uart->ier & UART_IER_TX_ENABLE) && (uart->lsr & UART_LSR_TX_EMPTY)) {
                iir = 0x02;  // 发送保持寄存器空中断
            }
            // Modem状态中断
            else if ((uart->ier & UART_IER_MODEM_STATUS) && (uart->msr & 0x0F)) {
                iir = 0x00;  // Modem状态中断
            }
            
            return iir;
        }
            
        case UART_LCR:
            return uart->lcr;
            
        case UART_MCR:
            return uart->mcr;
            
        case UART_LSR:
            return uart->lsr;
            
        case UART_MSR:
            return uart->msr;
            
        case UART_SCR:
            return uart->scr;
            
        default:
            return 0;
    }
}

void uart_receive(uart_t *uart, uint8_t data) {
    if (uart->rx_count >= UART_BUFFER_SIZE) {
        // 缓冲区满，设置溢出错误
        uart->lsr |= UART_LSR_OVERRUN;
        return;
    }
    
    uart->rx_buffer[uart->rx_head] = data;
    uart->rx_head = (uart->rx_head + 1) % UART_BUFFER_SIZE;
    uart->rx_count++;
    
    // 设置接收数据就绪标志
    uart->lsr |= UART_LSR_RX_READY;
    
    // 如果启用了接收中断，触发中断
    if (uart->ier & UART_IER_RX_ENABLE) {
        uart_update_interrupt(uart);
    }
}

uint8_t uart_transmit(uart_t *uart) {
    if (uart->tx_count == 0) {
        return 0;
    }
    
    uint8_t data = uart->tx_buffer[uart->tx_tail];
    uart->tx_tail = (uart->tx_tail + 1) % UART_BUFFER_SIZE;
    uart->tx_count--;
    
    if (uart->tx_count == 0) {
        uart->lsr |= UART_LSR_TX_EMPTY | UART_LSR_TX_IDLE;
        
        // 发送空中断
        if (uart->ier & UART_IER_TX_ENABLE) {
            uart_update_interrupt(uart);
        }
    }
    
    return data;
}

bool uart_can_read(const uart_t *uart) {
    return uart->rx_count > 0;
}

bool uart_can_write(const uart_t *uart) {
    return uart->tx_count < UART_BUFFER_SIZE;
}

void uart_set_tx_callback(uart_t *uart, void (*callback)(uint8_t)) {
    uart->tx_callback = callback;
}

void uart_update_interrupt(uart_t *uart) {
    if (uart->pic == NULL) return;
    
    // 检查是否有中断需要触发
    bool has_interrupt = false;
    
    // 接收数据中断
    if ((uart->ier & UART_IER_RX_ENABLE) && (uart->lsr & UART_LSR_RX_READY)) {
        has_interrupt = true;
    }
    // 发送空中断（只触发一次）
    else if ((uart->ier & UART_IER_TX_ENABLE) && (uart->lsr & UART_LSR_TX_EMPTY)) {
        has_interrupt = true;
    }
    // 线路状态中断
    else if ((uart->ier & UART_IER_LINE_STATUS) && (uart->lsr & 0x1E)) {
        has_interrupt = true;
    }
    
    if (has_interrupt) {
        pic_raise_irq(uart->pic, uart->irq);
    } else {
        pic_lower_irq(uart->pic, uart->irq);
    }
}
