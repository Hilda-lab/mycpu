#include "mycpu/cpu.h"

#include "mycpu/decoder.h"
#include "mycpu/exec.h"
#include "mycpu/mmu.h"
#include "mycpu/pic.h"
#include "mycpu/idt.h"

void cpu_init(cpu_t *cpu) {
    // 初始化所有寄存器
    cpu->eax = 0;
    cpu->ebx = 0;
    cpu->ecx = 0;
    cpu->edx = 0;
    cpu->esi = 0;
    cpu->edi = 0;
    cpu->ebp = 0;
    cpu->esp = 0;
    cpu->eip = 0;
    cpu->eflags = 0;
    
    // 初始化段寄存器
    cpu->cs = 0;
    cpu->ds = 0;
    cpu->es = 0;
    cpu->ss = 0;
    cpu->fs = 0;
    cpu->gs = 0;
    
    // 初始化控制寄存器
    cpu->cr0 = 0;
    cpu->cr2 = 0;
    cpu->cr3 = 0;
    cpu->cr4 = 0;
    
    // 初始化状态
    cpu->state = CPU_STOPPED;
    cpu->exception = CPU_EXC_NONE;
    
    // 初始化中断状态
    cpu->interrupts_enabled = false;
    cpu->interrupt_vector = 0;
    cpu->in_interrupt = false;
    cpu->saved_eip = 0;
    cpu->saved_eflags = 0;
    
    // 清除外设关联
    cpu->mmu = NULL;
    cpu->pic = NULL;
    cpu->idt = NULL;
}

void cpu_reset(cpu_t *cpu, uint32_t entry, uint32_t stack_top) {
    cpu->eax = 0;
    cpu->ebx = 0;
    cpu->ecx = 0;
    cpu->edx = 0;
    cpu->esi = 0;
    cpu->edi = 0;
    cpu->ebp = 0;
    cpu->esp = stack_top;
    cpu->eip = entry;
    cpu->eflags = 0;
    cpu->state = CPU_RUNNING;
    cpu->exception = CPU_EXC_NONE;
    cpu->interrupts_enabled = false;
    cpu->in_interrupt = false;
}

// 检查并处理中断
static void check_interrupts(cpu_t *cpu, memory_t *memory) {
    // 如果中断被禁用或正在处理中断，直接返回
    if (!cpu->interrupts_enabled || cpu->in_interrupt) {
        return;
    }
    
    // 检查PIC是否有待处理的中断
    if (cpu->pic && pic_has_pending(cpu->pic)) {
        int irq = pic_get_pending_irq(cpu->pic);
        if (irq >= 0) {
            uint8_t vector = pic_get_vector(cpu->pic, irq);
            
            // 保存上下文
            cpu->saved_eip = cpu->eip;
            cpu->saved_eflags = cpu->eflags;
            cpu->interrupt_vector = vector;
            cpu->in_interrupt = true;
            
            // 通知PIC开始处理中断
            pic_acknowledge(cpu->pic, irq);
            
            // 跳转到中断处理程序
            if (cpu->idt) {
                uint32_t handler_addr = idt_get_handler_addr(cpu->idt, vector);
                if (handler_addr != 0) {
                    cpu->eip = handler_addr;
                }
            }
        }
    }
}

void cpu_step(cpu_t *cpu, memory_t *memory) {
    instruction_t inst;

    if (cpu == NULL || memory == NULL) {
        return;
    }
    if (cpu->state != CPU_RUNNING) {
        return;
    }
    
    // 检查中断
    check_interrupts(cpu, memory);
    
    // 如果启用了分页且有MMU，需要使用虚拟地址翻译
    // 这里简化处理：如果MMU存在且分页启用，使用MMU读取指令
    // 但当前实现中，解码器直接访问内存，所以这里需要扩展
    
    if (!decode_instruction(memory, cpu->eip, &inst)) {
        cpu->state = CPU_EXCEPTION;
        cpu->exception = CPU_EXC_INVALID_OPCODE;
        return;
    }
    
    execute_instruction(cpu, memory, &inst);
}

void cpu_run(cpu_t *cpu, memory_t *memory, uint32_t max_steps) {
    uint32_t step = 0;
    while (cpu->state == CPU_RUNNING && step < max_steps) {
        cpu_step(cpu, memory);
        step++;
    }
}

void cpu_enable_interrupts(cpu_t *cpu) {
    cpu->interrupts_enabled = true;
    cpu->eflags |= EFLAGS_IF;
}

void cpu_disable_interrupts(cpu_t *cpu) {
    cpu->interrupts_enabled = false;
    cpu->eflags &= ~EFLAGS_IF;
}

bool cpu_are_interrupts_enabled(const cpu_t *cpu) {
    return cpu->interrupts_enabled;
}

void cpu_interrupt(cpu_t *cpu, memory_t *memory, uint8_t vector, uint32_t error_code) {
    // 保存当前状态
    cpu->saved_eip = cpu->eip;
    cpu->saved_eflags = cpu->eflags;
    cpu->interrupt_vector = vector;
    
    // 如果不是中断门，保存到栈上
    // 简化实现：总是保存到CPU结构中
    (void)memory;  // 避免未使用警告
    (void)error_code;
    
    // 标记为中断处理中
    cpu->in_interrupt = true;
    
    // 跳转到中断处理程序
    if (cpu->idt) {
        uint32_t handler_addr = idt_get_handler_addr(cpu->idt, vector);
        if (handler_addr != 0) {
            cpu->eip = handler_addr;
        } else {
            // 没有处理程序，触发双重故障
            cpu_raise_exception(cpu, CPU_EXC_DOUBLE_FAULT);
        }
    }
}

void cpu_iret(cpu_t *cpu, memory_t *memory) {
    (void)memory;  // 避免未使用警告
    
    // 恢复保存的状态
    cpu->eip = cpu->saved_eip;
    cpu->eflags = cpu->saved_eflags;
    cpu->in_interrupt = false;
    
    // 如果有关联的PIC，发送EOI
    if (cpu->pic && cpu->interrupt_vector >= 32) {
        uint8_t irq = cpu->interrupt_vector - 32;
        pic_end_of_interrupt(cpu->pic, irq);
    }
}

void cpu_set_mmu(cpu_t *cpu, struct mmu *mmu) {
    cpu->mmu = mmu;
}

void cpu_set_pic(cpu_t *cpu, struct pic *pic) {
    cpu->pic = pic;
}

void cpu_set_idt(cpu_t *cpu, struct idt *idt) {
    cpu->idt = idt;
}

void cpu_raise_exception(cpu_t *cpu, cpu_exception_t exc) {
    cpu->exception = exc;
    cpu->state = CPU_EXCEPTION;
    
    // 根据异常类型触发中断
    uint8_t vector = 0;
    switch (exc) {
        case CPU_EXC_DIVIDE_ERROR:
            vector = INT_DIVIDE_ERROR;
            break;
        case CPU_EXC_INVALID_OPCODE:
            vector = INT_INVALID_OPCODE;
            break;
        case CPU_EXC_PAGE_FAULT:
            vector = INT_PAGE_FAULT;
            break;
        case CPU_EXC_GENERAL_PROTECTION:
            vector = INT_GENERAL_PROTECTION;
            break;
        case CPU_EXC_DOUBLE_FAULT:
            vector = INT_DOUBLE_FAULT;
            break;
        default:
            return;
    }
    
    // 可以在这里触发异常中断
    // cpu_interrupt(cpu, memory, vector, 0);
}
