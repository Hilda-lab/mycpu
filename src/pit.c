#include "mycpu/pit.h"

#include <string.h>

void pit_init(pit_t *pit, pic_t *pic) {
    memset(pit, 0, sizeof(pit_t));
    pit->pic = pic;
    
    // 初始化所有通道
    for (int i = 0; i < 3; i++) {
        pit->channels[i].mode = PIT_MODE_SQUARE_WAVE;
        pit->channels[i].access = PIT_ACCESS_LSB_THEN_MSB;
        pit->channels[i].reload_value = 0;
        pit->channels[i].current_count = 0;
        pit->channels[i].gate = (i != 2);  // 通道2的gate需要单独设置
        pit->channels[i].out = true;  // 初始输出高电平
        pit->channels[i].read_state = 0;
        pit->channels[i].write_state = 0;
    }
}

void pit_write_control(pit_t *pit, uint8_t value) {
    // 控制字格式：
    // Bits 7-6: 通道选择
    // Bits 5-4: 访问模式
    // Bits 3-1: 工作模式
    // Bit 0: BCD模式
    
    uint8_t channel = (value >> 6) & 0x03;
    uint8_t access = (value >> 4) & 0x03;
    uint8_t mode = (value >> 1) & 0x07;
    bool bcd = (value & 0x01) != 0;
    
    // 读回命令（通道3）
    if (channel == 3) {
        // 读回命令，暂不实现
        return;
    }
    
    pit_channel_state_t *ch = &pit->channels[channel];
    
    // 锁存计数器命令
    if (access == PIT_ACCESS_LATCH_COUNT) {
        ch->latched = true;
        ch->latched_count = ch->current_count;
        return;
    }
    
    ch->access = access;
    ch->mode = mode;
    ch->bcd_mode = bcd;
    ch->write_state = 0;
    ch->read_state = 0;
}

void pit_write_channel(pit_t *pit, pit_channel_t channel, uint8_t value) {
    if (channel > 2) return;
    
    pit_channel_state_t *ch = &pit->channels[channel];
    
    switch (ch->access) {
        case PIT_ACCESS_LSB_ONLY:
            ch->reload_value = (ch->reload_value & 0xFF00) | value;
            ch->current_count = ch->reload_value;
            ch->write_state = 0;
            break;
            
        case PIT_ACCESS_MSB_ONLY:
            ch->reload_value = (ch->reload_value & 0x00FF) | (value << 8);
            ch->current_count = ch->reload_value;
            ch->write_state = 0;
            break;
            
        case PIT_ACCESS_LSB_THEN_MSB:
            if (ch->write_state == 0) {
                ch->write_latch = value;
                ch->write_state = 1;
            } else {
                ch->reload_value = ch->write_latch | (value << 8);
                ch->current_count = ch->reload_value;
                ch->write_state = 0;
            }
            break;
            
        default:
            break;
    }
}

uint8_t pit_read_channel(pit_t *pit, pit_channel_t channel) {
    if (channel > 2) return 0;
    
    pit_channel_state_t *ch = &pit->channels[channel];
    uint16_t count = ch->latched ? ch->latched_count : ch->current_count;
    
    switch (ch->access) {
        case PIT_ACCESS_LSB_ONLY:
            ch->latched = false;
            return count & 0xFF;
            
        case PIT_ACCESS_MSB_ONLY:
            ch->latched = false;
            return (count >> 8) & 0xFF;
            
        case PIT_ACCESS_LSB_THEN_MSB:
            if (ch->read_state == 0) {
                ch->read_state = 1;
                return count & 0xFF;
            } else {
                ch->read_state = 0;
                ch->latched = false;
                return (count >> 8) & 0xFF;
            }
            
        default:
            return 0;
    }
}

void pit_tick(pit_t *pit) {
    pit->ticks++;
    
    // 更新通道0（系统定时器）
    pit_channel_state_t *ch = &pit->channels[PIT_CHANNEL_0];
    
    if (ch->gate && ch->reload_value > 0) {
        ch->current_count--;
        
        if (ch->current_count == 0) {
            // 计数到0，重载
            ch->current_count = ch->reload_value;
            
            // 根据模式处理输出
            switch (ch->mode) {
                case PIT_MODE_INT_ON_TERMINAL_COUNT:
                    // 模式0：计数结束产生中断
                    ch->out = true;
                    if (pit->pic) {
                        pic_raise_irq(pit->pic, IRQ0_TIMER);
                    }
                    break;
                    
                case PIT_MODE_RATE_GENERATOR:
                    // 模式2：周期性中断
                    if (pit->pic) {
                        pic_raise_irq(pit->pic, IRQ0_TIMER);
                    }
                    break;
                    
                case PIT_MODE_SQUARE_WAVE:
                    // 模式3：方波，切换输出
                    ch->out = !ch->out;
                    if (pit->pic && ch->out) {
                        pic_raise_irq(pit->pic, IRQ0_TIMER);
                    }
                    break;
                    
                default:
                    break;
            }
        }
    }
}

uint64_t pit_get_milliseconds(const pit_t *pit) {
    // 假设PIT频率为标准1193182Hz
    // 如果reload_value为1193，则每毫秒约产生一次中断
    if (pit->channels[0].reload_value > 0) {
        uint32_t hz = PIT_FREQUENCY / pit->channels[0].reload_value;
        return pit->ticks / hz * 1000;
    }
    return 0;
}

void pit_set_frequency(pit_t *pit, pit_channel_t channel, uint32_t frequency) {
    if (channel > 2) return;
    if (frequency == 0) return;
    
    uint16_t divisor = (uint16_t)(PIT_FREQUENCY / frequency);
    if (divisor == 0) divisor = 1;
    
    // 写入除数（先低字节后高字节）
    pit->channels[channel].reload_value = divisor;
    pit->channels[channel].current_count = divisor;
}

uint32_t pit_get_frequency(const pit_t *pit, pit_channel_t channel) {
    if (channel > 2) return 0;
    if (pit->channels[channel].reload_value == 0) return 0;
    
    return PIT_FREQUENCY / pit->channels[channel].reload_value;
}
