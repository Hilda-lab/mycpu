#ifndef MYCPU_IDT_H
#define MYCPU_IDT_H

#include "mycpu/types.h"

// 中断向量号定义
#define INT_DIVIDE_ERROR        0   // 除零错误
#define INT_DEBUG               1   // 调试异常
#define INT_NMI                 2   // 不可屏蔽中断
#define INT_BREAKPOINT          3   // 断点
#define INT_OVERFLOW            4   // 溢出
#define INT_BOUND_RANGE         5   // 边界范围超出
#define INT_INVALID_OPCODE      6   // 无效操作码
#define INT_DEVICE_NOT_AVAIL    7   // 设备不可用
#define INT_DOUBLE_FAULT        8   // 双重故障
#define INT_COPROCESSOR         9   // 协处理器段超限
#define INT_INVALID_TSS         10  // 无效TSS
#define INT_SEGMENT_NOT_PRESENT 11  // 段不存在
#define INT_STACK_SEGMENT       12  // 栈段故障
#define INT_GENERAL_PROTECTION  13  // 一般保护错误
#define INT_PAGE_FAULT          14  // 页错误
#define INT_FPU_ERROR           16  // FPU错误
#define INT_ALIGNMENT_CHECK     17  // 对齐检查
#define INT_MACHINE_CHECK       18  // 机器检查
#define INT_SIMD_EXCEPTION      19  // SIMD异常

// 用户定义中断起始向量
#define INT_USER_START          32

// 最大中断向量数
#define IDT_MAX_ENTRIES         256

// 中断门类型
#define IDT_TYPE_INTERRUPT      0xE  // 中断门
#define IDT_TYPE_TRAP           0xF  // 陷阱门
#define IDT_TYPE_TASK           0x5  // 任务门

// IDT门描述符
typedef struct {
    uint16_t offset_low;      // 处理程序地址低16位
    uint16_t selector;        // 代码段选择子
    uint8_t  zero;            // 保留
    uint8_t  type_attr;       // 类型和属性
    uint16_t offset_high;     // 处理程序地址高16位
} __attribute__((packed)) idt_entry_t;

// IDT寄存器
typedef struct {
    uint16_t limit;           // IDT大小-1
    uint32_t base;            // IDT基址
} __attribute__((packed)) idtr_t;

// 中断上下文（保存在栈上的状态）
typedef struct {
    uint32_t eip;             // 指令指针
    uint32_t cs;              // 代码段（保留，暂不使用）
    uint32_t eflags;          // 标志寄存器
    // 如果是特权级变化，还会保存：
    uint32_t esp;             // 栈指针（可选）
    uint32_t ss;              // 栈段（可选）
} interrupt_frame_t;

// 中断处理函数类型
typedef void (*interrupt_handler_t)(uint8_t vector, uint32_t error_code);

// IDT表
typedef struct {
    idt_entry_t entries[IDT_MAX_ENTRIES];
    idtr_t idtr;
    interrupt_handler_t handlers[IDT_MAX_ENTRIES];
    uint32_t handler_addresses[IDT_MAX_ENTRIES];
} idt_t;

// 初始化IDT
void idt_init(idt_t *idt);

// 设置中断门
void idt_set_gate(idt_t *idt, uint8_t vector, uint32_t handler, 
                  uint16_t selector, uint8_t type_attr);

// 设置中断处理函数
void idt_set_handler(idt_t *idt, uint8_t vector, interrupt_handler_t handler);

// 获取IDTR值
void idt_get_idtr(const idt_t *idt, idtr_t *idtr);

// 加载IDT（在真实x86中用于lidt指令）
void idt_load(const idt_t *idt);

// 中断入口点（由汇编调用）
void idt_interrupt_entry(idt_t *idt, uint8_t vector, uint32_t error_code);

// 获取中断处理函数地址
uint32_t idt_get_handler_addr(const idt_t *idt, uint8_t vector);

#endif
