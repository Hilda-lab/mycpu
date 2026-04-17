#ifndef MYCPU_PIT_H
#define MYCPU_PIT_H

#include "mycpu/types.h"
#include "mycpu/pic.h"

// PIT时钟频率（8254标准频率）
#define PIT_FREQUENCY   1193182

// PIT通道
typedef enum {
    PIT_CHANNEL_0 = 0,      // 通道0 - 系统计时器
    PIT_CHANNEL_1 = 1,      // 通道1 - RAM刷新（通常不用）
    PIT_CHANNEL_2 = 2,      // 通道2 - 扬声器
} pit_channel_t;

// PIT工作模式
typedef enum {
    PIT_MODE_INT_ON_TERMINAL_COUNT = 0,    // 模式0 - 终端计数中断
    PIT_MODE_HARDWARE_RETRIGGERABLE_ONE_SHOT = 1,  // 模式1 - 硬件可重触发单稳态
    PIT_MODE_RATE_GENERATOR = 2,           // 模式2 - 速率发生器
    PIT_MODE_SQUARE_WAVE = 3,              // 模式3 - 方波发生器
    PIT_MODE_SOFTWARE_TRIGGERED_STROBE = 4, // 模式4 - 软件触发选通
    PIT_MODE_HARDWARE_TRIGGERED_STROBE = 5, // 模式5 - 硬件触发选通
} pit_mode_t;

// PIT访问模式
typedef enum {
    PIT_ACCESS_LATCH_COUNT = 0,   // 锁存计数值
    PIT_ACCESS_LSB_ONLY = 1,      // 只访问低字节
    PIT_ACCESS_MSB_ONLY = 2,      // 只访问高字节
    PIT_ACCESS_LSB_THEN_MSB = 3,  // 先低字节后高字节
} pit_access_t;

// PIT通道状态
typedef struct {
    uint16_t reload_value;    // 重载值
    uint16_t current_count;   // 当前计数值
    pit_mode_t mode;          // 工作模式
    pit_access_t access;      // 访问模式
    bool bcd_mode;            // 是否BCD模式
    bool gate;                // 门控信号
    bool out;                 // 输出信号
    bool latched;             // 计数是否已锁存
    uint16_t latched_count;   // 锁存的计数值
    uint8_t read_state;       // 读取状态（用于LSB/MSB序列）
    uint8_t write_state;      // 写入状态（用于LSB/MSB序列）
    uint16_t write_latch;     // 写入锁存
} pit_channel_state_t;

// PIT设备
typedef struct {
    pit_channel_state_t channels[3];
    pic_t *pic;               // 指向PIC的指针
    uint64_t ticks;           // 总时钟周期
} pit_t;

// 初始化PIT
void pit_init(pit_t *pit, pic_t *pic);

// 写入控制字
void pit_write_control(pit_t *pit, uint8_t value);

// 写入通道数据
void pit_write_channel(pit_t *pit, pit_channel_t channel, uint8_t value);

// 读取通道数据
uint8_t pit_read_channel(pit_t *pit, pit_channel_t channel);

// 时钟周期更新（每个CPU周期调用一次）
void pit_tick(pit_t *pit);

// 获取经过的时间（毫秒）
uint64_t pit_get_milliseconds(const pit_t *pit);

// 设置定时器频率
void pit_set_frequency(pit_t *pit, pit_channel_t channel, uint32_t frequency);

// 获取定时器频率
uint32_t pit_get_frequency(const pit_t *pit, pit_channel_t channel);

#endif
