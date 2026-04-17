#ifndef MYCPU_CPU_H
#define MYCPU_CPU_H

#include "mycpu/types.h"
#include "mycpu/memory.h"

// 不透明指针（避免循环依赖）
struct mmu;
struct pic;
struct idt;

typedef enum {
    CPU_STOPPED = 0,
    CPU_RUNNING,
    CPU_HALTED,
    CPU_EXCEPTION
} cpu_state_t;

// CPU异常类型
typedef enum {
    CPU_EXC_NONE = 0,
    CPU_EXC_DIVIDE_ERROR,
    CPU_EXC_INVALID_OPCODE,
    CPU_EXC_PAGE_FAULT,
    CPU_EXC_GENERAL_PROTECTION,
    CPU_EXC_DOUBLE_FAULT,
} cpu_exception_t;

typedef struct {
    // 通用寄存器
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    
    // 指令指针和标志寄存器
    uint32_t eip;
    uint32_t eflags;
    
    // 段寄存器（简化，暂未使用）
    uint16_t cs;
    uint16_t ds;
    uint16_t es;
    uint16_t ss;
    uint16_t fs;
    uint16_t gs;
    
    // 控制寄存器
    uint32_t cr0;
    uint32_t cr2;           // 页错误地址
    uint32_t cr3;           // 页目录基址
    uint32_t cr4;
    
    // CPU状态
    cpu_state_t state;
    cpu_exception_t exception;
    
    // 中断相关
    bool interrupts_enabled;    // IF标志
    uint8_t interrupt_vector;   // 当前中断向量
    bool in_interrupt;          // 是否在中断处理中
    uint32_t saved_eip;         // 中断时保存的EIP
    uint32_t saved_eflags;      // 中断时保存的EFLAGS
    
    // 关联的外设（不透明指针）
    struct mmu *mmu;
    struct pic *pic;
    struct idt *idt;
} cpu_t;

// 标志位定义 (x86 EFLAGS)
#define EFLAGS_CF   (1u << 0)   // Carry Flag
#define EFLAGS_PF   (1u << 2)   // Parity Flag
#define EFLAGS_AF   (1u << 4)   // Auxiliary Carry Flag
#define EFLAGS_ZF   (1u << 6)   // Zero Flag
#define EFLAGS_SF   (1u << 7)   // Sign Flag
#define EFLAGS_TF   (1u << 8)   // Trap Flag
#define EFLAGS_IF   (1u << 9)   // Interrupt Enable Flag
#define EFLAGS_DF   (1u << 10)  // Direction Flag
#define EFLAGS_OF   (1u << 11)  // Overflow Flag
#define EFLAGS_IOPL (3u << 12)  // I/O Privilege Level
#define EFLAGS_NT   (1u << 14)  // Nested Task
#define EFLAGS_RF   (1u << 16)  // Resume Flag
#define EFLAGS_VM   (1u << 17)  // Virtual 8086 Mode
#define EFLAGS_AC   (1u << 18)  // Alignment Check
#define EFLAGS_VIF  (1u << 19)  // Virtual Interrupt Flag
#define EFLAGS_VIP  (1u << 20)  // Virtual Interrupt Pending
#define EFLAGS_ID   (1u << 21)  // ID Flag

void cpu_init(cpu_t *cpu);
void cpu_reset(cpu_t *cpu, uint32_t entry, uint32_t stack_top);
void cpu_step(cpu_t *cpu, memory_t *memory);
void cpu_run(cpu_t *cpu, memory_t *memory, uint32_t max_steps);

// 中断相关
void cpu_enable_interrupts(cpu_t *cpu);
void cpu_disable_interrupts(cpu_t *cpu);
bool cpu_are_interrupts_enabled(const cpu_t *cpu);

// 触发中断
void cpu_interrupt(cpu_t *cpu, memory_t *memory, uint8_t vector, uint32_t error_code);

// 中断返回
void cpu_iret(cpu_t *cpu, memory_t *memory);

// 设置关联外设
void cpu_set_mmu(cpu_t *cpu, struct mmu *mmu);
void cpu_set_pic(cpu_t *cpu, struct pic *pic);
void cpu_set_idt(cpu_t *cpu, struct idt *idt);

// 触发异常
void cpu_raise_exception(cpu_t *cpu, cpu_exception_t exc);

#endif
