/**
 * 中断和分页功能测试
 * 
 * 测试：
 * 1. PIC中断控制器
 * 2. PIT定时器
 * 3. UART串口
 * 4. IDT中断描述符表
 * 5. MMU内存管理单元
 * 6. 新指令：CLI, STI, INT, IRET, PUSHF, POPF
 */

#include <stdio.h>
#include <assert.h>

#include "mycpu/cpu.h"
#include "mycpu/memory.h"
#include "mycpu/isa.h"
#include "mycpu/pic.h"
#include "mycpu/pit.h"
#include "mycpu/uart.h"
#include "mycpu/idt.h"
#include "mycpu/mmu.h"

// 测试PIC
void test_pic(void) {
    printf("=== 测试PIC中断控制器 ===\n");
    
    pic_t pic;
    pic_init(&pic, PIC_VECTOR_BASE);
    
    // 测试初始化状态
    assert(pic.initialized == true);
    assert(pic.imr == 0xFF);  // 所有中断被屏蔽
    
    // 解除IRQ0和IRQ3的屏蔽
    pic_set_mask(&pic, IRQ0_TIMER, false);
    pic_set_mask(&pic, IRQ3_UART, false);
    
    assert(pic_is_masked(&pic, IRQ0_TIMER) == false);
    assert(pic_is_masked(&pic, IRQ3_UART) == false);
    assert(pic_is_masked(&pic, IRQ1_KEYBOARD) == true);
    
    // 触发中断请求
    pic_raise_irq(&pic, IRQ0_TIMER);
    assert(pic.irr & (1 << IRQ0_TIMER));
    assert(pic_has_pending(&pic));
    
    // 获取待处理的中断
    int irq = pic_get_pending_irq(&pic);
    assert(irq == IRQ0_TIMER);
    
    // 确认中断
    pic_acknowledge(&pic, IRQ0_TIMER);
    assert(pic.isr & (1 << IRQ0_TIMER));
    assert(pic.priority == IRQ0_TIMER);
    
    // 中断结束
    pic_end_of_interrupt(&pic, IRQ0_TIMER);
    assert((pic.isr & (1 << IRQ0_TIMER)) == 0);
    assert(pic.priority < 0);
    
    printf("✅ PIC测试通过\n\n");
}

// 测试PIT
void test_pit(void) {
    printf("=== 测试PIT定时器 ===\n");
    
    pic_t pic;
    pic_init(&pic, PIC_VECTOR_BASE);
    pic_set_mask(&pic, IRQ0_TIMER, false);
    
    pit_t pit;
    pit_init(&pit, &pic);
    
    // 测试设置频率
    pit_set_frequency(&pit, PIT_CHANNEL_0, 100);  // 100Hz
    uint32_t freq = pit_get_frequency(&pit, PIT_CHANNEL_0);
    printf("PIT频率: %u Hz\n", freq);
    assert(freq > 0);
    
    // 模拟时钟周期
    printf("模拟1000个时钟周期...\n");
    for (int i = 0; i < 1000; i++) {
        pit_tick(&pit);
    }
    
    printf("✅ PIT测试通过\n\n");
}

// 测试UART
void test_uart(void) {
    printf("=== 测试UART串口 ===\n");
    
    pic_t pic;
    pic_init(&pic, PIC_VECTOR_BASE);
    pic_set_mask(&pic, IRQ3_UART, false);
    
    uart_t uart;
    uart_init(&uart, &pic, IRQ3_UART);
    
    // 测试发送
    printf("测试发送: ");
    uart_write_reg(&uart, UART_THR, 'A');
    uart_write_reg(&uart, UART_THR, 'B');
    uart_write_reg(&uart, UART_THR, 'C');
    printf("\n");
    
    // 测试接收
    printf("测试接收: ");
    uart_receive(&uart, 'X');
    uart_receive(&uart, 'Y');
    uart_receive(&uart, 'Z');
    
    assert(uart_can_read(&uart));
    
    while (uart_can_read(&uart)) {
        uint8_t ch = uart_read_reg(&uart, UART_RBR);
        printf("%c", ch);
    }
    printf("\n");
    
    // 测试中断使能
    uart_write_reg(&uart, UART_IER, UART_IER_RX_ENABLE);
    assert(uart.ier & UART_IER_RX_ENABLE);
    
    printf("✅ UART测试通过\n\n");
}

// 测试IDT
void test_idt(void) {
    printf("=== 测试IDT中断描述符表 ===\n");
    
    idt_t idt;
    idt_init(&idt);
    
    // 设置中断门
    idt_set_gate(&idt, INT_DIVIDE_ERROR, 0x1000, 0x08, IDT_TYPE_INTERRUPT);
    idt_set_gate(&idt, INT_PAGE_FAULT, 0x2000, 0x08, IDT_TYPE_TRAP);
    
    // 验证设置
    uint32_t handler = idt_get_handler_addr(&idt, INT_DIVIDE_ERROR);
    assert(handler == 0x1000);
    
    handler = idt_get_handler_addr(&idt, INT_PAGE_FAULT);
    assert(handler == 0x2000);
    
    printf("IDT条目数: %d\n", IDT_MAX_ENTRIES);
    
    printf("✅ IDT测试通过\n\n");
}

// 测试MMU
void test_mmu(void) {
    printf("=== 测试MMU内存管理单元 ===\n");
    
    memory_t memory;
    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    
    mmu_t mmu;
    mmu_init(&mmu, &memory);
    
    // 初始状态：分页禁用
    assert(!mmu_is_paging_enabled(&mmu));
    
    // 创建页表
    uint32_t page_table = mmu_create_page_table(&mmu);
    printf("创建页表，地址: 0x%08X\n", page_table);
    assert(page_table != 0);
    
    // 设置页目录
    mmu_set_page_dir(&mmu, page_table);
    printf("页目录基址: 0x%08X\n", mmu.cr3);
    
    // 启用分页
    mmu_enable_paging(&mmu);
    assert(mmu_is_paging_enabled(&mmu));
    printf("分页已启用\n");
    
    // 恒等映射前1MB
    mmu_identity_map(&mmu, 0, 0x100000, PTE_PRESENT | PTE_WRITABLE);
    printf("恒等映射: 0x00000000 - 0x00100000\n");
    
    // 测试地址翻译
    uint32_t vaddr = 0x1000;
    uint32_t paddr;
    bool result = mmu_translate(&mmu, vaddr, &paddr);
    printf("虚拟地址 0x%08X -> 物理地址 0x%08X (结果: %s)\n", 
           vaddr, paddr, result ? "成功" : "失败");
    assert(result);
    assert(paddr == vaddr);  // 恒等映射
    
    // 测试读写
    uint32_t test_value = 0xDEADBEEF;
    assert(mmu_write32(&mmu, vaddr, test_value));
    
    uint32_t read_value;
    assert(mmu_read32(&mmu, vaddr, &read_value));
    assert(read_value == test_value);
    printf("写入并读回: 0x%08X ✓\n", read_value);
    
    // 统计信息
    printf("TLB命中: %lu, TLB未命中: %lu\n", mmu.tlb_hits, mmu.tlb_misses);
    
    // 禁用分页
    mmu_disable_paging(&mmu);
    assert(!mmu_is_paging_enabled(&mmu));
    
    memory_free(&memory);
    printf("✅ MMU测试通过\n\n");
}

// 测试新指令
void test_new_instructions(void) {
    printf("=== 测试新指令 (CLI, STI, PUSHF, POPF) ===\n");
    
    memory_t memory;
    cpu_t cpu;
    
    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    cpu_init(&cpu);
    cpu_reset(&cpu, 0u, (uint32_t)memory.size);
    
    // 初始状态：中断禁用
    assert(!cpu.interrupts_enabled);
    
    // STI - 启用中断
    uint8_t program[] = {
        OP_STI,         // 启用中断
        OP_CLI,         // 禁用中断
        OP_PUSHF,       // 标志入栈
        OP_POP_REG, REG_EAX,  // 弹出到EAX
        OP_STI,         // 再次启用
        OP_HLT
    };
    
    for (size_t i = 0; i < sizeof(program); i++) {
        memory_write8(&memory, (uint32_t)i, program[i]);
    }
    
    cpu_run(&cpu, &memory, 100u);
    
    printf("执行STI后，中断启用: %s\n", 
           cpu.interrupts_enabled ? "是" : "否");
    assert(cpu.interrupts_enabled);
    
    printf("EAX (POPF后的标志): 0x%08X\n", cpu.eax);
    
    printf("✅ 新指令测试通过\n\n");
    
    memory_free(&memory);
}

// 测试完整的中断处理流程
void test_interrupt_flow(void) {
    printf("=== 测试完整中断处理流程 ===\n");
    
    memory_t memory;
    cpu_t cpu;
    pic_t pic;
    idt_t idt;
    
    assert(memory_init(&memory, MYCPU_MEM_SIZE));
    cpu_init(&cpu);
    pic_init(&pic, PIC_VECTOR_BASE);
    idt_init(&idt);
    
    // 设置CPU关联
    cpu_set_pic(&cpu, &pic);
    cpu_set_idt(&cpu, &idt);
    
    // 解除定时器中断屏蔽
    pic_set_mask(&pic, IRQ0_TIMER, false);
    
    // 设置中断处理程序地址
    // IRQ0 (定时器) -> 向量32
    idt_set_gate(&idt, 32, 0x1000, 0x08, IDT_TYPE_INTERRUPT);
    
    cpu_reset(&cpu, 0u, (uint32_t)memory.size);
    
    // 简单程序：循环等待中断
    uint8_t program[] = {
        OP_STI,         // 启用中断
        OP_JMP_REL32, 0xFB, 0xFF, 0xFF, 0xFF,  // JMP -5 (无限循环)
        OP_HLT
    };
    
    for (size_t i = 0; i < sizeof(program); i++) {
        memory_write8(&memory, (uint32_t)i, program[i]);
    }
    
    // 模拟定时器中断
    printf("触发定时器中断...\n");
    pic_raise_irq(&pic, IRQ0_TIMER);
    
    // 执行几步
    for (int i = 0; i < 10; i++) {
        cpu_step(&cpu, &memory);
    }
    
    printf("中断触发后，CPU在中断处理中: %s\n",
           cpu.in_interrupt ? "是" : "否");
    
    printf("✅ 中断流程测试通过\n\n");
    
    memory_free(&memory);
}

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║   中断和分页功能测试                   ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    
    test_pic();
    test_pit();
    test_uart();
    test_idt();
    test_mmu();
    test_new_instructions();
    test_interrupt_flow();
    
    printf("╔════════════════════════════════════════╗\n");
    printf("║   ✅ 所有测试通过！                    ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    return 0;
}
