#include "mycpu/pic.h"

#include <string.h>

void pic_init(pic_t *pic, uint8_t vector_base) {
    memset(pic, 0, sizeof(pic_t));
    pic->icw2 = vector_base;
    pic->initialized = true;
    pic->priority = -1;  // 无正在服务的中断
    // 默认屏蔽所有中断
    pic->imr = 0xFF;
}

void pic_set_mask(pic_t *pic, uint8_t irq, bool mask) {
    if (irq >= IRQ_IRQ_MAX) return;
    
    if (mask) {
        pic->imr |= (1 << irq);
    } else {
        pic->imr &= ~(1 << irq);
    }
}

bool pic_is_masked(const pic_t *pic, uint8_t irq) {
    if (irq >= IRQ_IRQ_MAX) return true;
    return (pic->imr & (1 << irq)) != 0;
}

void pic_raise_irq(pic_t *pic, uint8_t irq) {
    if (irq >= IRQ_IRQ_MAX) return;
    
    // 设置IRR中的对应位
    pic->irr |= (1 << irq);
}

void pic_lower_irq(pic_t *pic, uint8_t irq) {
    if (irq >= IRQ_IRQ_MAX) return;
    
    // 清除IRR中的对应位
    pic->irr &= ~(1 << irq);
}

int pic_get_pending_irq(const pic_t *pic) {
    // 找到IRR中未被屏蔽的最高优先级中断
    // 优先级：IRQ0最高，IRQ7最低
    uint8_t pending = pic->irr & (~pic->imr);
    
    // 还要检查是否有更高优先级的中断正在服务
    // 如果有更高优先级的中断在服务，则不能响应低优先级中断
    if (pic->priority >= 0) {
        // 屏蔽优先级低于当前服务中断的IRQ
        pending &= (0xFF >> (8 - pic->priority));
    }
    
    if (pending == 0) return -1;
    
    // 找到最低位的1（最高优先级）
    for (int i = 0; i < 8; i++) {
        if (pending & (1 << i)) {
            return i;
        }
    }
    
    return -1;
}

uint8_t pic_get_vector(const pic_t *pic, uint8_t irq) {
    return pic->icw2 + irq;
}

void pic_acknowledge(pic_t *pic, uint8_t irq) {
    if (irq >= IRQ_IRQ_MAX) return;
    
    // 将IRR中的位移动到ISR
    pic->irr &= ~(1 << irq);
    pic->isr |= (1 << irq);
    
    // 更新当前优先级
    if (pic->priority < 0 || irq < pic->priority) {
        pic->priority = irq;
    }
}

void pic_end_of_interrupt(pic_t *pic, uint8_t irq) {
    if (irq >= IRQ_IRQ_MAX) return;
    
    // 从ISR中移除
    pic->isr &= ~(1 << irq);
    
    // 如果没有其他中断在服务，重置优先级
    if (pic->isr == 0) {
        pic->priority = -1;
    } else {
        // 找到ISR中最高优先级的中断
        for (int i = 0; i < 8; i++) {
            if (pic->isr & (1 << i)) {
                pic->priority = i;
                break;
            }
        }
    }
}

bool pic_has_pending(const pic_t *pic) {
    return pic_get_pending_irq(pic) >= 0;
}

uint8_t pic_get_irr(const pic_t *pic) {
    return pic->irr;
}

uint8_t pic_get_isr(const pic_t *pic) {
    return pic->isr;
}
