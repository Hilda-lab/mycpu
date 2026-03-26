# myCPU 项目状态与测试指南

## 📊 当前项目状态

### ✅ 已完成功能（完成度：约 35%）

#### 1. CPU 核心模块 ✅
- 8 个 32 位通用寄存器（EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP）
- 指令指针 EIP（程序计数器）
- 标志寄存器 EFLAGS（部分实现，仅 ZF 标志）
- CPU 状态管理（STOPPED, RUNNING, HALTED, EXCEPTION）
- 主循环：取指→译码→执行

#### 2. 已实现的指令（10 条）✅
| 指令类型 | 指令 | 功能 | 状态 |
|---------|------|------|------|
| 数据传送 | `MOV reg, imm32` | 立即数 → 寄存器 | ✅ |
| 数据传送 | `MOV reg, reg` | 寄存器 → 寄存器 | ✅ |
| 算术运算 | `ADD reg, reg` | 寄存器相加 | ✅ |
| 算术运算 | `SUB reg, reg` | 寄存器相减 | ✅ |
| 比较运算 | `CMP reg, reg` | 比较（设置 ZF） | ✅ |
| 控制流 | `JMP rel32` | 无条件跳转 | ✅ |
| 控制流 | `JZ rel32` | 相等时跳转 | ✅ |
| 控制流 | `JNZ rel32` | 不相等时跳转 | ✅ |
| 栈操作 | `PUSH reg` | 寄存器压栈 | ✅ |
| 栈操作 | `POP reg` | 寄存器出栈 | ✅ |
| 系统指令 | `HLT` | 停止 CPU | ✅ |

#### 3. 内存系统 ✅
- 16 MB 平坦内存空间
- 边界检查和异常处理
- 8/16/32 位读写支持
- 小端序字节序

#### 4. 测试系统 ✅
- 自动化测试框架（CTest）
- 3 个测试程序：
  - `mycpu_tests`：自动化单元测试
  - `test_simple`：简单功能演示（6 个测试）
  - `test_detailed`：详细执行过程演示

---

## 🧪 测试步骤

### 快速测试（推荐）
```bash
# 运行所有测试
./run_all_tests.sh
```

这个脚本会自动运行：
1. 自动化测试
2. 主程序测试
3. 简单功能测试

---

### 单独测试

#### 1. 自动化测试
```bash
ctest --test-dir build --output-on-failure
```

**预期结果**：
```
100% tests passed, 0 tests failed out of 1
```

**测试内容**：
- 比较和条件跳转（CMP + JZ）
- 栈操作和算术运算（PUSH + POP + ADD + SUB）
- 无条件跳转（JMP）

---

#### 2. 主程序测试
```bash
./build/mycpu
```

**预期结果**：
```
OK
myCPU demo done, EAX=42 EIP=0x00000007 state=2
```

**程序内容**：
```c
MOV EAX, 42
HLT
```

**结果解释**：
- `EAX=42`：寄存器 EAX 的值是 42 ✅
- `EIP=0x00000007`：指令指针停在地址 7（执行了 2 条指令，共 7 字节）
- `state=2`：CPU 状态为 HALTED（已停止）

---

#### 3. 简单功能测试
```bash
./build/test_simple
```

**包含 6 个测试**：

1. **MOV 指令测试**
   - 程序：`MOV EAX, 50; MOV EBX, 100; HLT`
   - 预期：`EAX=50, EBX=100`
   - 用途：测试寄存器赋值

2. **ADD 指令测试**
   - 程序：`MOV EAX, 10; MOV EBX, 20; ADD EAX, EBX; HLT`
   - 预期：`EAX=30, EBX=20`
   - 用途：测试寄存器加法

3. **SUB 指令测试**
   - 程序：`MOV EAX, 100; MOV EBX, 30; SUB EAX, EBX; HLT`
   - 预期：`EAX=70, EBX=30`
   - 用途：测试寄存器减法

4. **CMP 和 JZ 指令测试**
   - 程序：`MOV EAX, 25; MOV EBX, 25; CMP EAX, EBX; JZ +5; ...`
   - 预期：`ECX=1, ZF=1`
   - 用途：测试比较和条件跳转

5. **PUSH 和 POP 指令测试**
   - 程序：`MOV EAX, 200; PUSH EAX; POP EBX; HLT`
   - 预期：`EAX=200, EBX=200, ESP 回到原值`
   - 用途：测试栈操作

6. **综合测试**
   - 程序：计算 `(10 + 20) - 15`
   - 预期：`EAX=15`
   - 用途：测试综合算术运算

---

#### 4. 详细执行测试（高级）
```bash
./build/test_detailed
```

这个测试会显示：
- 每一条指令的执行过程
- 执行前后的寄存器状态
- EIP（指令指针）的变化
- 标志位（ZF）的状态

**适合用于**：
- 理解 CPU 的工作原理
- 调试程序执行过程
- 学习指令的执行细节

---

## 📖 代码功能解释

### CPU 模块 (`src/cpu.c`)
**作用**：模拟 CPU 的核心执行流程

**比喻**：
- 寄存器 = 8 个小抽屉，可以存数字
- EIP = 读书时的手指，指到哪一行就读哪一行
- EFLAGS = 记本，记录运算结果的状态（是否为 0 等）

**关键函数**：
```c
void cpu_init(cpu_t *cpu);        // 初始化 CPU（清空所有寄存器）
void cpu_reset(cpu_t *cpu, ...); // 重置 CPU（设置 EIP 和 ESP）
void cpu_step(cpu_t *cpu, ...);  // 执行一条指令
void cpu_run(cpu_t *cpu, ...);   // 运行多步指令
```

---

### 解码器模块 (`src/decoder.c`)
**作用**：将内存中的二进制指令翻译成人类可理解的格式

**示例**：
```
内存中的字节：0x10 0x00 0x2A 0x00 0x00 0x00
解码后：MOV EAX, 42
```

**比喻**：就像翻译官，把机器语言翻译成人能懂的指令

---

### 执行模块 (`src/exec.c`)
**作用**：实际执行指令，修改寄存器和内存

**示例**：
```
指令：ADD EAX, EBX
执行：EAX = EAX + EBX
```

**比喻**：就像工人，按照指令完成实际工作

---

### 内存模块 (`src/memory.c`)
**作用**：模拟 16 MB 的内存空间

**功能**：
- 提供内存读写接口
- 检查地址是否越界
- 支持不同大小的读写（8/16/32 位）

**比喻**：就像一个巨大的仓库，可以存储数据

---

## ✅ 如何判断测试是否通过？

### 正常现象：
1. ✅ 所有测试都有 `✓` 标记
2. ✅ 寄存器值与预期相符
3. ✅ CPU 状态为 `CPU_HALTED`（state=2）
4. ✅ 自动化测试显示 `100% tests passed`

### 异常现象：
1. ❌ 寄存器值与预期不符
2. ❌ CPU 状态为 `CPU_EXCEPTION`（state=3）
3. ❌ 测试失败提示

---

## 🎯 接下来应该做什么？

### 阶段 1：扩展基础指令集（优先级：⭐⭐⭐⭐⭐）

#### 待实现指令（20+ 条）：

**按位运算指令**（建议先实现）：
```c
OP_AND_REG_REG  // AND：EAX = EAX & EBX (按位与)
OP_OR_REG_REG   // OR：EAX = EAX | EBX (按位或)
OP_XOR_REG_REG  // XOR：EAX = EAX ^ EBX (按位异或)
OP_NOT_REG      // NOT：EAX = ~EAX (按位取反)
```

**立即数算术指令**：
```c
OP_ADD_REG_IMM  // ADD：EAX = EAX + 42
OP_SUB_REG_IMM  // SUB：EAX = EAX - 42
OP_AND_REG_IMM  // AND：EAX = EAX & 42
OP_OR_REG_IMM   // OR：EAX = EAX | 42
OP_XOR_REG_IMM  // XOR：EAX = EAX ^ 42
```

**移位指令**：
```c
OP_SHL_REG_REG  // SHL：EAX = EAX << EBX (左移)
OP_SHR_REG_REG  // SHR：EAX = EAX >> EBX (右移)
```

**单操作数指令**：
```c
OP_INC_REG      // INC：EAX = EAX + 1
OP_DEC_REG      // DEC：EAX = EAX - 1
OP_NEG_REG      // NEG：EAX = -EAX (取负)
```

**内存访问指令**：
```c
OP_MOV_REG_MEM  // MOV：EAX = [EBX] (从内存读取)
OP_MOV_MEM_REG  // MOV：[EBX] = EAX (写入内存)
OP_LEA          // LEA：加载有效地址
```

**控制流指令**：
```c
OP_CALL_REL32   // CALL：调用子程序
OP_RET          // RET：从子程序返回
OP_LOOP         // LOOP：循环指令
```

**测试指令**：
```c
OP_TEST_REG_REG // TEST：逻辑测试（设置 ZF）
OP_CLC          // CLC：清进位标志
OP_STC          // STC：置进位标志
```

---

### 阶段 2：完善标志位（优先级：⭐⭐⭐⭐）

#### 待实现的标志位：
```c
#define FLAG_CF (1u << 0)   // 进位标志（加减法是否进位/借位）
#define FLAG_PF (1u << 2)   // 奇偶标志（结果中 1 的个数是否为偶数）
#define FLAG_AF (1u << 4)   // 辅助进位标志（低 4 位是否进位）
#define FLAG_SF (1u << 7)   // 符号标志（结果是否为负数）
#define FLAG_OF (1u << 11)  // 溢出标志（有符号数是否溢出）
```

**需要更新**：
- ADD/SUB 指令：设置 CF, PF, AF, SF, OF
- CMP 指令：设置所有相关标志位
- 测试程序：验证标志位是否正确

---

### 阶段 3：实现内存访问（优先级：⭐⭐⭐）

**需要实现**：
1. 修改解码器，支持内存操作数
2. 实现寄存器间接寻址
3. 添加内存访问测试

**示例程序**：
```assembly
MOV EAX, 100      ; EAX = 100
MOV EBX, 0x1000   ; EBX = 0x1000 (内存地址)
MOV [EBX], EAX    ; 将 EAX 的值存入地址 0x1000
MOV ECX, [EBX]    ; 从地址 0x1000 读取值到 ECX
; 此时 ECX = 100
```

---

### 阶段 4：实现 CALL/RET（优先级：⭐⭐⭐）

**需要实现**：
1. CALL 指令：将返回地址压栈，跳转到目标地址
2. RET 指令：从栈中弹出返回地址，跳转回去

**示例程序**：
```assembly
CALL my_function  ; 调用子程序
MOV EAX, 100      ; 这条指令会在子程序返回后执行
JMP end

my_function:
    MOV EBX, 200  ; 子程序代码
    RET           ; 返回

end:
    HLT
```

---

### 阶段 5：中断与异常（优先级：⭐⭐）

**需要实现**：
1. 中断控制器（PIC 模拟）
2. 中断入口和上下文保存
3. 异常处理机制

---

### 阶段 6：外设接口（优先级：⭐⭐）

**需要实现**：
1. 完善 UART（支持输入、中断）
2. 添加定时器（PIT）
3. 统一硬件接口规范

---

## 📚 学习建议

### 初学者：
1. ✅ 从简单测试开始：`./build/test_simple`
2. ✅ 观察详细执行：`./build/test_detailed`
3. ✅ 修改测试程序：编辑 `test_simple.c`，创建自己的测试
4. ✅ 阅读源代码：从 `src/main.c` 开始

### 进阶学习：
1. 📖 理解 x86 指令集架构
2. 📖 学习 CPU 流水线原理
3. 📖 研究内存管理机制
4. 📖 了解中断和异常处理

### 实践建议：
1. 🔧 一次只添加一条新指令
2. ✅ 为每条新指令编写测试
3. 📝 记录学习笔记
4. 🤝 遇到问题及时请教

---

## 🛠️ 常用命令

```bash
# 编译项目
cmake -S . -B build
cmake --build build -j

# 运行所有测试
./run_all_tests.sh

# 单独运行测试
ctest --test-dir build --output-on-failure
./build/mycpu
./build/test_simple
./build/test_detailed

# 清理重新编译
rm -rf build
cmake -S . -B build
cmake --build build -j

# 查看帮助文档
cat TESTING_GUIDE.md
```

---

## 📋 课程项目要求对照

### 必完成（已完成 35%）：
- ✅ CPU 核心模块（寄存器、PC、标志位）
- ✅ 基础指令集（10 条，目标 30~40 条）
- ✅ 流水线主循环（取指→译码→执行）
- ⏸️ 内存模块（已完成基础版，待添加 MMU）
- ⏸️ 边界检查、越界异常（已完成基础版）
- ⏸️ MMU/分页机制（未开始）

### 加分项（未开始）：
- ⏸️ 中断控制器模拟
- ⏸️ 异常入口、上下文保存与恢复
- ⏸️ 时钟中断（为操作系统预留）
- ⏸️ UART 串口（已完成基础输出，待完善输入和中断）
- ⏸️ 定时器中断
- ⏸️ 统一硬件接口规范

---

## 🎉 总结

你的 myCPU 项目已经有了一个坚实的基础：
- ✅ CPU 核心功能完整
- ✅ 基础指令集可用
- ✅ 测试系统完善
- ✅ 代码结构清晰

**下一步建议**：
1. 从简单的指令开始（AND, OR, XOR）
2. 逐步完善标志位
3. 添加内存访问支持
4. 实现 CALL/RET

**预计完成时间**：
- 扩展到 30 条指令：1-2 周
- 完善标志位：1 周
- 内存访问：1 周
- CALL/RET：1 周
- 中断和外设：2-3 周

祝你学习顺利！🚀
