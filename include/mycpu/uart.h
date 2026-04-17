#ifndef MYCPU_UART_H
#define MYCPU_UART_H

#include "mycpu/types.h"
#include "mycpu/pic.h"

// UART寄存器偏移
#define UART_RBR    0   // 接收缓冲寄存器（读）
#define UART_THR    0   // 发送保持寄存器（写）
#define UART_IER    1   // 中断使能寄存器
#define UART_IIR    2   // 中断标识寄存器（读）
#define UART_FCR    2   // FIFO控制寄存器（写）
#define UART_LCR    3   // 线路控制寄存器
#define UART_MCR    4   // Modem控制寄存器
#define UART_LSR    5   // 线路状态寄存器
#define UART_MSR    6   // Modem状态寄存器
#define UART_SCR    7   // 暂存寄存器

// 中断使能寄存器位
#define UART_IER_RX_ENABLE      (1 << 0)   // 接收数据中断
#define UART_IER_TX_ENABLE      (1 << 1)   // 发送空中断
#define UART_IER_LINE_STATUS    (1 << 2)   // 线路状态中断
#define UART_IER_MODEM_STATUS   (1 << 3)   // Modem状态中断

// 线路状态寄存器位
#define UART_LSR_RX_READY       (1 << 0)   // 接收数据就绪
#define UART_LSR_OVERRUN        (1 << 1)   // 溢出错误
#define UART_LSR_PARITY_ERR     (1 << 2)   // 奇偶校验错误
#define UART_LSR_FRAME_ERR      (1 << 3)   // 帧错误
#define UART_LSR_BREAK_IND      (1 << 4)   // 中断指示
#define UART_LSR_TX_EMPTY       (1 << 5)   // 发送保持寄存器空
#define UART_LSR_TX_IDLE        (1 << 6)   // 发送器空闲
#define UART_LSR_FIFO_ERR       (1 << 7)   // FIFO错误

// FIFO控制寄存器位
#define UART_FCR_ENABLE         (1 << 0)   // 启用FIFO
#define UART_FCR_CLEAR_RX       (1 << 1)   // 清除接收FIFO
#define UART_FCR_CLEAR_TX       (1 << 2)   // 清除发送FIFO
#define UART_FCR_DMA            (1 << 3)   // DMA模式选择
#define UART_FCR_TRIGGER_1      (0 << 6)   // 触发级别：1字节
#define UART_FCR_TRIGGER_4      (1 << 6)   // 触发级别：4字节
#define UART_FCR_TRIGGER_8      (2 << 6)   // 触发级别：8字节
#define UART_FCR_TRIGGER_14     (3 << 6)   // 触发级别：14字节

#define UART_BUFFER_SIZE 16

// UART设备
typedef struct {
    // 寄存器
    uint8_t rbr;            // 接收缓冲
    uint8_t thr;            // 发送保持
    uint8_t ier;            // 中断使能
    uint8_t iir;            // 中断标识
    uint8_t fcr;            // FIFO控制
    uint8_t lcr;            // 线路控制
    uint8_t mcr;            // Modem控制
    uint8_t lsr;            // 线路状态
    uint8_t msr;            // Modem状态
    uint8_t scr;            // 暂存
    uint8_t dll;            // 除数锁存低字节
    uint8_t dlm;            // 除数锁存高字节
    
    // FIFO缓冲区
    uint8_t rx_buffer[UART_BUFFER_SIZE];
    uint8_t tx_buffer[UART_BUFFER_SIZE];
    uint8_t rx_count;       // 接收缓冲区字节数
    uint8_t tx_count;       // 发送缓冲区字节数
    uint8_t rx_head;
    uint8_t tx_head;
    uint8_t rx_tail;
    uint8_t tx_tail;
    
    // 关联的PIC和IRQ
    pic_t *pic;
    uint8_t irq;
    
    // 回调函数
    void (*tx_callback)(uint8_t ch);   // 发送回调
} uart_t;

// 初始化UART
void uart_init(uart_t *uart, pic_t *pic, uint8_t irq);

// 写入寄存器
void uart_write_reg(uart_t *uart, uint8_t reg, uint8_t value);

// 读取寄存器
uint8_t uart_read_reg(uart_t *uart, uint8_t reg);

// 接收数据（从外部输入）
void uart_receive(uart_t *uart, uint8_t data);

// 发送数据（立即发送）
uint8_t uart_transmit(uart_t *uart);

// 检查是否有数据可读
bool uart_can_read(const uart_t *uart);

// 检查是否可以发送
bool uart_can_write(const uart_t *uart);

// 设置发送回调
void uart_set_tx_callback(uart_t *uart, void (*callback)(uint8_t));

// 更新中断状态
void uart_update_interrupt(uart_t *uart);

#endif
