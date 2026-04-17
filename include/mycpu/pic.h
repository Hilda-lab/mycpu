#ifndef MYCPU_PIC_H
#define MYCPU_PIC_H

#include "mycpu/types.h"

// 中断向量号定义
#define IRQ0_TIMER      0   // 定时器中断
#define IRQ1_KEYBOARD   1   // 键盘中断
#define IRQ2_CASCADE    2   // 级联
#define IRQ3_UART       3   // UART串口中断
#define IRQ4_UART2      4   // UART2中断
#define IRQ_IRQ_MAX     16

// 中断向量号基址
#define PIC_VECTOR_BASE 32

// PIC状态标志
#define PIC_FLAG_IRQ_PENDING   (1 << 0)
#define PIC_FLAG_IRQ_MASKED    (1 << 1)

typedef struct {
    uint8_t irr;            // 中断请求寄存器
    uint8_t isr;            // 中断服务寄存器
    uint8_t imr;            // 中断屏蔽寄存器
    uint8_t icw1;           // 初始化控制字1
    uint8_t icw2;           // 初始化控制字2（向量基址）
    uint8_t icw3;           // 初始化控制字3
    uint8_t icw4;           // 初始化控制字4
    uint8_t ocw3;           // 操作控制字3
    bool initialized;       // 是否已初始化
    int8_t priority;        // 当前优先级（正在服务的中断）
} pic_t;

// 初始化PIC
void pic_init(pic_t *pic, uint8_t vector_base);

// 设置中断屏蔽
void pic_set_mask(pic_t *pic, uint8_t irq, bool mask);

// 获取中断屏蔽状态
bool pic_is_masked(const pic_t *pic, uint8_t irq);

// 触发中断请求
void pic_raise_irq(pic_t *pic, uint8_t irq);

// 清除中断请求
void pic_lower_irq(pic_t *pic, uint8_t irq);

// 获取待处理的最高优先级中断（返回IRQ号，-1表示无中断）
int pic_get_pending_irq(const pic_t *pic);

// 获取中断向量号
uint8_t pic_get_vector(const pic_t *pic, uint8_t irq);

// 开始处理中断（将IRQ移到ISR）
void pic_acknowledge(pic_t *pic, uint8_t irq);

// 结束中断处理（从ISR移除）
void pic_end_of_interrupt(pic_t *pic, uint8_t irq);

// 检查是否有待处理的中断
bool pic_has_pending(const pic_t *pic);

// 获取IRR状态
uint8_t pic_get_irr(const pic_t *pic);

// 获取ISR状态
uint8_t pic_get_isr(const pic_t *pic);

#endif
