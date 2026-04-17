#include "mycpu/idt.h"

#include <string.h>

void idt_init(idt_t *idt) {
    memset(idt, 0, sizeof(idt_t));
    
    // 设置IDTR
    idt->idtr.limit = sizeof(idt->entries) - 1;
    idt->idtr.base = (uint32_t)idt->entries;
    
    // 设置默认中断处理函数地址
    for (int i = 0; i < IDT_MAX_ENTRIES; i++) {
        idt->handler_addresses[i] = 0;  // 无处理函数
    }
}

void idt_set_gate(idt_t *idt, uint8_t vector, uint32_t handler,
                  uint16_t selector, uint8_t type_attr) {
    idt->entries[vector].offset_low = handler & 0xFFFF;
    idt->entries[vector].offset_high = (handler >> 16) & 0xFFFF;
    idt->entries[vector].selector = selector;
    idt->entries[vector].zero = 0;
    idt->entries[vector].type_attr = type_attr;
    idt->handler_addresses[vector] = handler;
}

void idt_set_handler(idt_t *idt, uint8_t vector, interrupt_handler_t handler) {
    if (vector < IDT_MAX_ENTRIES) {
        idt->handlers[vector] = handler;
    }
}

void idt_get_idtr(const idt_t *idt, idtr_t *idtr) {
    idtr->limit = idt->idtr.limit;
    idtr->base = idt->idtr.base;
}

void idt_load(const idt_t *idt) {
    // 在真实x86中，这会执行lidt指令
    // 在模拟器中，我们只是记录IDT已加载
    (void)idt;
}

void idt_interrupt_entry(idt_t *idt, uint8_t vector, uint32_t error_code) {
    // 调用注册的中断处理函数
    if (vector < IDT_MAX_ENTRIES && idt->handlers[vector] != NULL) {
        idt->handlers[vector](vector, error_code);
    }
}

uint32_t idt_get_handler_addr(const idt_t *idt, uint8_t vector) {
    if (vector < IDT_MAX_ENTRIES) {
        return idt->handler_addresses[vector];
    }
    return 0;
}
