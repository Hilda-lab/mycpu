# myCPU 项目架构详解

## 📁 项目整体结构

```
mycpu/
├── include/mycpu/          # 头文件（模块接口定义）
├── src/                     # 实现文件（核心逻辑）
├── tests/                   # 测试代码
└── build/                   # 编译输出
```

---

## 🏗️ 一、CPU核心模块

### 1.1 寄存器与CPU状态
**文件**: `include/mycpu/cpu.h` (行 29-73)

```c
typedef struct {
    // 通用寄存器（8个32位）
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    
    // 指令指针和标志寄存器
    uint32_t eip;       // 程序计数器
    uint32_t eflags;    // 标志寄存器
    
    // 控制寄存器（MMU分页用）
    uint32_t cr0, cr2, cr3, cr4;
    
    // 中断相关
    bool interrupts_enabled;
    uint8_t interrupt_vector;
    bool in_interrupt;
    
    // 关联外设
    struct mmu *mmu;
    struct pic *pic;
    struct idt *idt;
} cpu_t;
```

**标志位定义** (cpu.h 行 76-92):
```c
#define EFLAGS_CF   (1u << 0)   // 进位标志
#define EFLAGS_ZF   (1u << 6)   // 零标志
#define EFLAGS_SF   (1u << 7)   // 符号标志
#define EFLAGS_IF   (1u << 9)   // 中断使能标志 ⭐
#define EFLAGS_OF   (1u << 11)  // 溢出标志
```

### 1.2 指令集定义
**文件**: `include/mycpu/isa.h`

指令编码分配：
| 范围 | 指令类型 | 示例 |
|------|---------|------|
| 0x10-0x1F | 数据传送 | MOV |
| 0x20-0x2F | 算术运算 | ADD, SUB, CMP |
| 0x28-0x2F | 逻辑运算 | AND, OR, XOR, NOT |
| 0x38-0x3F | 移位指令 | SHL, SHR |
| 0x40-0x4F | 控制流 | JMP, JZ, CALL, RET |
| 0x50-0x5F | 栈操作 | PUSH, POP |
| 0x60-0x6F | 中断相关 | INT, IRET, CLI, STI |

### 1.3 CPU执行流程
**文件**: `src/cpu.c`

核心函数：
```c
void cpu_reset(cpu_t *cpu, uint32_t entry, uint32_t stack_top);
// 初始化CPU，重置所有寄存器

void cpu_step(cpu_t *cpu, memory_t *memory);
// ⭐ 核心函数：取指→译码→执行
// 1. 从EIP读取指令 (decoder.c)
// 2. 解析指令 (decoder.c)
// 3. 执行指令 (exec.c)
```

---

## 📋 二、指令执行详解

### 2.1 取指-译码-执行
**文件**: `src/exec.c` (643行)

执行函数入口 (exec.c 行 11-24):
```c
void cpu_exec_instruction(cpu_t *cpu, memory_t *memory, instruction_t *inst) {
    uint32_t next_eip = cpu->eip + inst->length;
    
    switch(inst->opcode) {
        case OP_MOV_REG_IMM:  // ... 处理 MOV reg, imm32
        case OP_MOV_REG_REG:  // ... 处理 MOV reg, reg
        case OP_ADD_REG_REG:   // ... 处理 ADD reg, reg
        // ... 更多指令
    }
}
```

### 2.2 各指令实现

| 指令 | 核心代码位置 | 功能说明 |
|------|------------|---------|
| **MOV** | exec.c 行 26-70 | 数据传送 |
| **ADD/SUB** | exec.c 行 72-130 | 算术运算 + 标志位更新 |
| **CMP** | exec.c 行 132-150 | 比较并设置标志位 |
| **JMP/JZ** | exec.c 行 152-210 | 条件/无条件跳转 |
| **CALL/RET** | exec.c 行 212-280 | 函数调用和返回 |
| **PUSH/POP** | exec.c 行 282-340 | 栈操作 |
| **AND/OR/XOR** | exec.c 行 342-430 | 逻辑运算 |
| **SHL/SHR** | exec.c 行 432-470 | 移位运算 |
| **INT/IRET** | exec.c 行 500-550 | 中断指令 |

### 2.3 标志位更新示例
**文件**: `src/exec.c` (行 90-130)

```c
// ADD指令执行时更新标志位
result = (uint32_t)((int32_t)a + (int32_t)b);

// 更新零标志 (ZF)
cpu->eflags = (cpu->eflags & ~FLAG_ZF) | 
              ((result == 0) ? FLAG_ZF : 0);

// 更新符号标志 (SF)
cpu->eflags = (cpu->eflags & ~FLAG_SF) | 
              ((result & 0x80000000) ? FLAG_SF : 0);

// 更新进位标志 (CF)
cpu->eflags = (cpu->eflags & ~FLAG_CF) | 
              ((result < a) ? FLAG_CF : 0);

// 更新溢出标志 (OF)
cpu->eflags = (cpu->eflags & ~FLAG_OF) | 
              (((int32_t)result < 0 && a > 0 && b > 0) ? FLAG_OF : 0);
```

---

## 💾 三、内存系统

### 3.1 内存模块接口
**文件**: `include/mycpu/memory.h`

```c
bool memory_init(memory_t *mem, size_t size);
bool memory_read8/16/32(memory_t *mem, uint32_t addr, uint8_t/16/32 *value);
bool memory_write8/16/32(memory_t *mem, uint32_t addr, uint8_t/16/32 value);
```

### 3.2 边界检查
**文件**: `src/memory.c` (行 30-60)

```c
bool memory_read8(memory_t *mem, uint32_t addr, uint8_t *value) {
    // ⭐ 边界检查：如果地址超出范围，返回false
    if (addr >= mem->size) {
        return false;  // 触发CPU_EXCEPTION
    }
    *value = mem->data[addr];
    return true;
}
```

### 3.3 内存布局（OS准备）
**文件**: `include/mycpu/memory.h` (行 6)

```c
#define MYCPU_MEM_SIZE 0x1000000  // 16MB
```

**内存布局建议** (OS开发时):
```
0x00000000 - 0x00000FFF   : 初始中断向量表 (4KB)
0x00001000 - 0x00004FFF   : 内核栈区域 (16KB)
0x00005000 - 0x0000FFFF   : 内核代码区域 (48KB)
0x00010000 - 0x000FFFFFFF : 用户空间 (剩余部分)
```

---

## 🔄 四、中断系统

### 4.1 中断控制器 (PIC)
**文件**: `include/mycpu/pic.h` + `src/pic.c`

**PIC结构** (pic.h 行 21-32):
```c
typedef struct {
    uint8_t irr;    // 中断请求寄存器 (Interrupt Request Register)
    uint8_t isr;    // 中断服务寄存器 (In-Service Register)
    uint8_t imr;    // 中断屏蔽寄存器 (Interrupt Mask Register)
    bool initialized;
    int8_t priority;
} pic_t;
```

**核心函数** (pic.c):
| 函数 | 功能 |
|------|------|
| `pic_init()` | 初始化PIC，设置中断向量基址 |
| `pic_raise_irq()` | 触发IRQ请求 |
| `pic_get_pending_irq()` | 获取最高优先级中断 |
| `pic_acknowledge()` | 确认中断，开始处理 |
| `pic_end_of_interrupt()` | 结束中断处理 |

**PIC工作流程** (pic.c):
```
硬件中断 → IRR置位 → PIC判断优先级 → 
→ CPU响应 → ISR置位 → CPU执行中断处理 → EOI → ISR清除
```

### 4.2 定时器 (PIT)
**文件**: `include/mycpu/pit.h` + `src/pit.c`

**8254 PIT** (pit.h 行 10-20):
```c
typedef struct {
    uint16_t count[3];           // 3个通道的计数器
    uint16_t initial[3];          // 初始计数值
    uint32_t frequency[3];       // 频率
    pic_t *pic;                  // 关联的PIC
} pit_t;
```

**工作模式** (pit.h 行 6-9):
- 模式0: 终端计数
- 模式2: 频率发生器（⭐ OS时钟节拍）
- 模式3: 方波发生器

### 4.3 中断描述符表 (IDT)
**文件**: `include/mycpu/idt.h` + `src/idt.c`

**IDT门描述符** (idt.h 行 23-35):
```c
typedef struct {
    uint16_t offset_low;     // 处理程序地址低16位
    uint16_t selector;       // 代码段选择子
    uint8_t istack;          // 中断栈表
    uint8_t type_attr;       // 类型和属性
    uint16_t offset_mid;     // 处理程序地址中16位
    uint32_t offset_high;    // 处理程序地址高32位
    uint32_t zero;
} idt_gate_t;
```

**IDT类型** (idt.h 行 40-45):
```c
#define IDT_TYPE_INTERRUPT  0x8E   // 中断门（自动禁用中断）
#define IDT_TYPE_TRAP       0x8F   // 陷阱门（保持中断状态）
```

### 4.4 CPU中断处理
**文件**: `src/cpu.c` (行 140-180)

```c
void cpu_interrupt(cpu_t *cpu, memory_t *memory, 
                   uint8_t vector, uint32_t error_code) {
    // ⭐ 1. 禁用中断
    cpu->interrupts_enabled = false;
    
    // ⭐ 2. 从IDT获取处理程序地址
    uint32_t handler = idt_get_handler_addr(cpu->idt, vector);
    
    // ⭐ 3. 保存上下文到栈
    stack_push32(cpu, memory, cpu->eflags);
    stack_push32(cpu, memory, cpu->cs);
    stack_push32(cpu, memory, cpu->eip);
    if (error_code != 0) {
        stack_push32(cpu, memory, error_code);
    }
    
    // ⭐ 4. 跳转到中断处理程序
    cpu->eip = handler;
    cpu->in_interrupt = true;
}

void cpu_iret(cpu_t *cpu, memory_t *memory) {
    // ⭐ 1. 从栈恢复上下文
    uint32_t eip, cs, eflags;
    stack_pop32(cpu, memory, &eip);
    stack_pop32(cpu, memory, &cs);
    stack_pop32(cpu, memory, &eflags);
    
    // ⭐ 2. 恢复状态
    cpu->eip = eip;
    cpu->cs = (uint16_t)cs;
    cpu->eflags = eflags;
    cpu->in_interrupt = false;
    
    // ⭐ 3. 重新启用中断
    cpu->interrupts_enabled = true;
}
```

---

## 🗂️ 五、MMU内存管理

### 5.1 MMU结构
**文件**: `include/mycpu/mmu.h` (行 47-82)

```c
typedef struct {
    uint32_t cr0;           // 分页启用位 (CR0.PG)
    uint32_t cr2;           // 页错误线性地址
    uint32_t cr3;           // 页目录物理地址
    uint64_t tlb_hits;      // TLB命中计数
    uint64_t tlb_misses;    // TLB未命中计数
    
    // ⭐ TLB缓存 (64项)
    struct {
        uint32_t virtual_addr;
        uint32_t physical_addr;
        bool valid;
    } tlb[64];
} mmu_t;
```

### 5.2 虚拟地址翻译
**文件**: `src/mmu.c` (行 100-150)

```c
bool mmu_translate(const mmu_t *mmu, uint32_t vaddr, uint32_t *paddr) {
    // ⭐ 检查分页是否启用
    if (!mmu_is_paging_enabled(mmu)) {
        *paddr = vaddr;  // 直通模式
        return true;
    }
    
    // ⭐ 虚拟地址分解
    uint32_t page_dir_index = (vaddr >> 22) & 0x3FF;    // 高10位
    uint32_t page_table_index = (vaddr >> 12) & 0x3FF; // 中10位
    uint32_t offset = vaddr & 0xFFF;                    // 低12位
    
    // ⭐ 1. 查询页目录 (CR3)
    pde_t pde;
    memory_read32(mmu->memory, mmu->cr3 + page_dir_index*4, &pde.value);
    if (!pde_is_present(pde)) {
        mmu->cr2 = vaddr;  // 保存错误地址
        return false;       // 页错误！
    }
    
    // ⭐ 2. 查询页表
    uint32_t page_table_addr = pde_get_frame(pde);
    pte_t pte;
    memory_read32(mmu->memory, page_table_addr + page_table_index*4, &pte.value);
    if (!pte_is_present(pte)) {
        mmu->cr2 = vaddr;  // 保存错误地址
        return false;       // 页错误！
    }
    
    // ⭐ 3. 合成物理地址
    *paddr = pte_get_frame(pte) | offset;
    return true;
}
```

### 5.3 二级页表结构
**文件**: `mmu.h` (行 7-16)

```
虚拟地址 (32位) 结构:
┌────────────────┬────────────────┬────────────────┐
│   页目录索引    │    页表索引     │    页内偏移     │
│   (10位)       │    (10位)      │    (12位)      │
│ 31      22     │ 21       12    │ 11        0    │
└────────────────┴────────────────┴────────────────┘
        ↓                  ↓               ↓
┌────────────────┐  ┌────────────────┐
│   页目录项      │  │    页表项       │
│  (1024项)      │  │   (1024项)     │
│               │→│               │→ 物理页框 (4KB)
└────────────────┘  └────────────────┘
```

### 5.4 恒等映射
**文件**: `src/mmu.c` (行 200-220)

```c
void mmu_identity_map(mmu_t *mmu, uint32_t start, 
                     uint32_t end, uint32_t flags) {
    // ⭐ 将虚拟地址直接映射到相同的物理地址
    // 用于OS内核：虚拟地址0xC0000000 → 物理地址0x00000000
    for (uint32_t vaddr = start; vaddr < end; vaddr += PAGE_SIZE) {
        mmu_map_page(mmu, vaddr, vaddr, flags);
    }
}
```

---

## 📡 六、UART串口

### 6.1 UART结构
**文件**: `include/mycpu/uart.h` (行 30-50)

```c
typedef struct {
    uint8_t thr;     // 发送保持寄存器
    uint8_t rbr;     // 接收缓冲寄存器
    uint8_t ier;      // 中断使能寄存器
    uint8_t iir;      // 中断标识寄存器
    uint8_t lcr;      // 线路控制寄存器
    uint8_t mcr;      // Modem控制寄存器
    uint8_t lsr;      // 线路状态寄存器
    uint8_t msr;      // Modem状态寄存器
    
    uint8_t tx_fifo[16];  // 发送FIFO
    uint8_t rx_fifo[16];  // 接收FIFO
    int tx_head, tx_tail;
    int rx_head, rx_tail;
} uart_t;
```

### 6.2 UART寄存器
**文件**: `uart.h` (行 15-28)

| 偏移 | 寄存器 | 功能 |
|------|--------|------|
| 0x00 | RBR/THR | 接收/发送缓冲 |
| 0x01 | IER | 中断使能 |
| 0x02 | IIR/FCR | 中断ID/先进先出控制 |
| 0x03 | LCR | 线路控制（数据位、校验、停止位）|
| 0x04 | MCR | Modem控制 |
| 0x05 | LSR | 线路状态（接收就绪、发送空）|
| 0x06 | MSR | Modem状态 |

---

## 🛠️ 七、汇编器

### 7.1 汇编器入口
**文件**: `src/asm_main.c`

### 7.2 汇编器核心
**文件**: `src/assembler.c` (27KB)

| 功能 | 核心函数 | 说明 |
|------|---------|------|
| 词法分析 | `asm_tokenize()` | 拆分指令和操作数 |
| 符号解析 | `asm_first_pass()` | 处理标签，计算地址 |
| 代码生成 | `asm_second_pass()` | 生成机器码 |
| 指令编码 | `asm_encode_instruction()` | 单条指令编码 |

### 7.3 汇编器支持指令
**文件**: `src/assembler.c` (行 200-600)

```
支持格式:
  MOV EAX, 100        ; 寄存器 ← 立即数
  MOV EAX, EBX        ; 寄存器 ← 寄存器
  MOV [EAX], EBX      ; 内存 ← 寄存器
  ADD EAX, 10         ; 加立即数
  JMP label          ; 跳转到标签
  CALL func          ; 调用函数
  LOOP label         ; 循环
```

---

## 🐛 八、调试器

### 8.1 调试器入口
**文件**: `src/debugger_main.c`

### 8.2 调试器核心
**文件**: `src/debugger.c` (614行)

| 功能 | 核心函数 | 说明 |
|------|---------|------|
| 界面绘制 | `debug_draw()` | ncurses界面 |
| 命令解析 | `debug_handle_command()` | 处理用户输入 |
| 断点管理 | `debug_set_breakpoint()` | 设置断点 |
| 单步执行 | `debug_step()` | 单步/继续运行 |

### 8.3 调试器功能
**文件**: `debugger.c` (行 100-200)

```c
// 支持的命令:
// R - 运行 (Continue)
// S - 单步 (Step)
// B addr - 设置断点
// D addr - 删除断点
// P reg - 打印寄存器
// M addr - 查看内存
// Q - 退出
```

---

## 🎯 八、项目如何拓展为操作系统

### 8.1 当前项目基础

你现在拥有的组件（OS开发所需）：

| 组件 | OS中的用途 | 当前状态 |
|------|----------|---------|
| **CPU** | 处理器抽象 | ✅ 完整 |
| **内存管理** | 虚拟内存、分页 | ✅ 完整（MMU）|
| **中断处理** | 系统调用、异常 | ✅ 完整（PIC/IDT）|
| **定时器** | 时钟节拍、调度 | ✅ 完整（PIT）|
| **串口** | 控制台输出 | ✅ 完整（UART）|

### 8.2 拓展路线图

```
第一阶段：裸机程序
┌─────────────────────────────────────────────────┐
│  1. 引导加载 (Bootloader)                        │
│     - 从固定地址加载内核                         │
│     - 建立临时页表                               │
│     - 跳转到内核入口                             │
│                                                 │
│  2. 内核入口点                                   │
│     - 初始化GDT (全局描述符表)                   │
│     - 设置栈指针                                 │
│     - 初始化PIC，启用中断                         │
│     - 打印 "Hello, OS!"                          │
└─────────────────────────────────────────────────┘

第二阶段：内核基础
┌─────────────────────────────────────────────────┐
│  3. 内存管理扩展                                 │
│     - 实现物理内存分配器 (bitmap/链表)            │
│     - 实现虚拟内存分配器 (slab/slab)              │
│     - 实现内存映射 (mmap)                         │
│                                                 │
│  4. 中断处理完善                                 │
│     - 设置完整的IDT                               │
│     - 实现异常处理（除零、页错误等）              │
│     - 实现系统调用 (INT 0x80)                     │
└─────────────────────────────────────────────────┘

第三阶段：进程管理
┌─────────────────────────────────────────────────┐
│  5. 进程/线程管理                                │
│     - 进程控制块 (PCB)                          │
│     - 上下文切换                                 │
│     - 调度器（轮转、优先级）                     │
│                                                 │
│  6. 进程间通信                                   │
│     - 管道 (pipe)                               │
│     - 信号 (signal)                             │
│     - 共享内存                                   │
└─────────────────────────────────────────────────┘

第四阶段：文件系统
┌─────────────────────────────────────────────────┐
│  7. 文件系统                                     │
│     - FAT32/Ext2 实现                           │
│     - 文件描述符抽象                             │
│     - 目录操作                                   │
│                                                 │
│  8. 设备驱动                                     │
│     - 键盘驱动                                  │
│     - VGA显示驱动                               │
│     - 磁盘驱动                                   │
└─────────────────────────────────────────────────┘
```

### 8.3 具体的代码修改

#### 8.3.1 添加GDT支持

新建 `src/gdt.c`:

```c
// 全局描述符表 (GDT)
// OS需要区分内核态和用户态

typedef struct {
    uint16_t limit_low;     // 段界限低16位
    uint16_t base_low;      // 基址低16位
    uint8_t base_mid;        // 基址中8位
    uint8_t access;          // 访问权限
    uint8_t granularity;     // 粒度
    uint8_t base_high;       // 基址高8位
} gdt_entry_t;

// GDT条目
#define GDT_ENTRY(code, base, limit, present, privilege) \
    { .base_low = (base) & 0xFFFF, \
      .base_mid = ((base) >> 16) & 0xFF, \
      .base_high = ((base) >> 24) & 0xFF, \
      .limit_low = (limit) & 0xFFFF, \
      .granularity = ((limit) >> 16) & 0x0F | 0xC0, \
      .access = present | (privilege << 5) | 0x92 }
```

#### 8.3.2 添加系统调用

修改 `src/exec.c`，添加系统调用指令:

```c
case OP_INT_IMM8:
    // 系统调用：INT 0x80
    if (inst->imm8 == 0x80) {
        // 从EAX获取系统调用号
        // 从EBX/ECX/EDX获取参数
        // 调用系统调用处理函数
        cpu->eax = syscalls[cpu->eax](cpu->ebx, cpu->ecx, cpu->edx);
    } else {
        cpu_interrupt(cpu, memory, inst->imm8, 0);
    }
    return;
```

#### 8.3.3 添加用户模式

修改 `cpu.h`，添加特权级字段:

```c
typedef struct {
    // ... 现有字段 ...
    uint8_t cpl;  // Current Privilege Level (0=内核, 3=用户)
} cpu_t;
```

#### 8.3.4 实现进程调度

新建 `src/scheduler.c`:

```c
// 进程控制块 (PCB)
typedef struct process {
    uint32_t pid;              // 进程ID
    uint32_t state;            // 进程状态
    uint32_t regs[8];          // 通用寄存器快照
    uint32_t eip, esp, eflags; // CPU状态
    uint32_t cr3;              // 页目录
    struct process *next;      // 链表指针
} pcb_t;

// 调度器
void scheduler_tick(void) {
    // 保存当前进程上下文
    current->eip = cpu.eip;
    current->esp = cpu.esp;
    current->eflags = cpu.eflags;
    
    // 选择下一个进程
    current = current->next;
    
    // 恢复新进程上下文
    cpu.eip = current->eip;
    cpu.esp = current->esp;
    cpu.eflags = current->eflags;
    cpu.cr3 = current->cr3;
}
```

### 8.4 推荐的参考资源

1. **OSDev Wiki** - 操作系统开发权威指南
   - https://wiki.osdev.org

2. **Linux内核源码** - 学习真实的操作系统实现

3. **《操作系统真象还原》** - 中文操作系统开发书籍

4. **《Writing a Simple Operating System from Scratch》** - Nick Blundell

### 8.5 里程碑示例

```
v1.0 (当前)    : 裸机模拟器，运行用户程序
    ↓
v2.0          : 添加GDT、简单的内核代码
    ↓
v3.0          : 实现物理内存分配器
    ↓
v4.0          : 实现进程调度（协作式）
    ↓
v5.0          : 实现虚拟内存和系统调用
    ↓
v6.0          : 添加文件系统
    ↓
v7.0          : 添加网络支持
```

---

## 📚 九、核心代码速查表

| 功能 | 头文件 | 源文件 | 核心函数 |
|------|--------|--------|---------|
| CPU核心 | cpu.h | cpu.c | cpu_step(), cpu_reset() |
| 指令执行 | isa.h, decoder.h | exec.c, decoder.c | cpu_exec_instruction() |
| 内存管理 | memory.h | memory.c | memory_read/write() |
| MMU分页 | mmu.h | mmu.c | mmu_translate() |
| 中断控制 | pic.h | pic.c | pic_raise_irq(), pic_get_pending_irq() |
| 定时器 | pit.h | pit.c | pit_tick(), pit_set_frequency() |
| 中断描述符 | idt.h | idt.c | idt_set_gate(), idt_get_handler() |
| 串口通信 | uart.h | uart.c | uart_write_reg(), uart_read_reg() |
| 汇编器 | assembler.h | assembler.c | asm_assemble() |
| 调试器 | debugger.h | debugger.c | debug_run(), debug_step() |

---

## 🎓 十、学习建议

1. **先理解CPU执行流程**: 阅读 `cpu.c` 的 `cpu_step()` 函数
2. **深入一条指令**: 选择ADD指令，理解取指→译码→执行→更新标志位
3. **理解中断机制**: 重点阅读 `cpu_interrupt()` 和 `cpu_iret()`
4. **理解分页**: 阅读 `mmu_translate()` 的虚拟地址翻译过程
5. **扩展实验**: 尝试添加一条新指令，如 `MUL` 乘法指令

---

**文档版本**: v1.0  
**最后更新**: 2026-04-17
