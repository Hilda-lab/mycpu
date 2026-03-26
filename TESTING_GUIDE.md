# myCPU 测试指南

## 📋 项目概述

myCPU 是一个用 C 语言实现的 x86 子集指令集模拟器。它可以模拟 CPU 的基本功能，包括寄存器、内存、指令解码和执行。

## 🧪 测试方法

### 方法 1：运行自动化测试

```bash
# 编译项目
cmake -S . -B build
cmake --build build -j

# 运行自动化测试
ctest --test-dir build --output-on-failure
```

**预期输出**：
```
Test project /home/hilda/mycpu/build
    Start 1: mycpu_tests
1/1 Test #1: mycpu_tests ......................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 1
```

---

### 方法 2：运行主程序

```bash
./build/mycpu
```

**程序内容**：
```c
uint8_t program[] = {
    OP_MOV_REG_IMM, REG_EAX, 0x2A, 0x00, 0x00, 0x00,  // MOV EAX, 42
    OP_HLT                                              // HLT (停止)
};
```

**预期输出**：
```
OK
myCPU demo done, EAX=42 EIP=0x00000007 state=2
```

**解释**：
- `OK`：程序成功执行
- `EAX=42`：EAX 寄存器的值是 42
- `EIP=0x00000007`：指令指针停在地址 7（执行了 2 条指令）
- `state=2`：CPU 状态是 CPU_HALTED（已停止）

---

### 方法 3：运行简单测试（推荐新手）

```bash
./build/test_simple
```

这个测试程序会演示 6 个基础功能：

#### 测试 1：MOV 指令
**功能**：将数字放入寄存器

**程序**：
```assembly
MOV EAX, 50
MOV EBX, 100
HLT
```

**预期结果**：
```
执行前：EAX=0, EBX=0
执行后：EAX=50 ✓, EBX=100 ✓
```

**解释**：
- MOV 指令将立即数 50 放入 EAX 寄存器
- MOV 指令将立即数 100 放入 EBX 寄存器
- 两个寄存器的值都正确设置

---

#### 测试 2：ADD 指令
**功能**：寄存器相加

**程序**：
```assembly
MOV EAX, 10
MOV EBX, 20
ADD EAX, EBX  ; EAX = EAX + EBX = 10 + 20 = 30
HLT
```

**预期结果**：
```
执行前：EAX=0, EBX=0
执行后：EAX=30 ✓ (应为 30), EBX=20
```

**解释**：
- ADD 指令将 EBX 的值加到 EAX 上
- EAX 从 10 变成 30，EBX 保持不变

---

#### 测试 3：SUB 指令
**功能**：寄存器相减

**程序**：
```assembly
MOV EAX, 100
MOV EBX, 30
SUB EAX, EBX  ; EAX = EAX - EBX = 100 - 30 = 70
HLT
```

**预期结果**：
```
执行前：EAX=100, EBX=30
执行后：EAX=70 ✓ (应为 70), EBX=30
```

**解释**：
- SUB 指令从 EAX 中减去 EBX 的值
- EAX 从 100 变成 70

---

#### 测试 4：CMP 和 JZ 指令
**功能**：比较和条件跳转

**程序**：
```assembly
MOV EAX, 25
MOV EBX, 25
CMP EAX, EBX  ; 比较，设置 ZF=1 (因为相等)
JZ +5        ; 如果 ZF=1，跳过 5 字节
MOV ECX, 0   ; 这条被跳过
MOV ECX, 1   ; 执行这条
HLT
```

**预期结果**：
```
执行前：EAX=0, EBX=0, ECX=0
执行后：EAX=25, EBX=25, ECX=1 ✓ (应为 1)
         ZF=1 ✓ (应为 1，因为 25 == 25)
```

**解释**：
- CMP 指令比较 EAX 和 EBX，因为相等，设置 ZF 标志为 1
- JZ 指令检测到 ZF=1，所以跳过下一条指令
- 程序跳过 "MOV ECX, 0"，直接执行 "MOV ECX, 1"

---

#### 测试 5：PUSH 和 POP 指令
**功能**：栈操作

**程序**：
```assembly
MOV EAX, 200
MOV EBX, 0
PUSH EAX      ; 将 EAX (200) 压入栈
POP EBX       ; 从栈弹出值到 EBX
HLT
```

**预期结果**：
```
执行前：EAX=0, EBX=0, ESP=16777216
执行后：EAX=200, EBX=200 ✓ (应为 200), ESP=16777216 ✓ (回到原值)
```

**解释**：
- PUSH 指令将 EAX 的值（200）压入栈中，ESP 减小 4
- POP 指令从栈中弹出值到 EBX，ESP 增加 4
- EBX 得到 200，ESP 回到初始值

---

#### 测试 6：综合测试
**功能**：计算 (10 + 20) - 15

**程序**：
```assembly
MOV EAX, 10
MOV EBX, 20
ADD EAX, EBX  ; EAX = 30
MOV EBX, 15
SUB EAX, EBX  ; EAX = 30 - 15 = 15
HLT
```

**预期结果**：
```
执行前：EAX=0
执行后：EAX=15 ✓ (应为 15)
```

**解释**：
- 程序完成了一次简单的算术运算
- 最终 EAX = (10 + 20) - 15 = 15

---

### 方法 4：运行详细测试（查看每一步）

```bash
./build/test_detailed
```

这个测试会显示 CPU 的每一步执行过程，包括：
- 当前执行的指令
- 执行前后的寄存器状态
- EIP（指令指针）的变化
- 标志位（ZF）的状态

---

## 📊 测试结果总结

### ✅ 已正常工作的功能

1. **寄存器操作**
   - 8 个 32 位通用寄存器（EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP）
   - 指令指针 EIP
   - 标志寄存器 EFLAGS（部分）

2. **数据传送指令**
   - `MOV reg, imm32`：寄存器 ← 立即数
   - `MOV reg, reg`：寄存器 ← 寄存器

3. **算术逻辑指令**
   - `ADD reg, reg`：寄存器相加
   - `SUB reg, reg`：寄存器相减
   - `CMP reg, reg`：比较（设置 ZF 标志）

4. **控制流指令**
   - `JMP rel32`：无条件跳转
   - `JZ rel32`：零标志跳转（相等时跳转）
   - `JNZ rel32`：非零标志跳转（不相等时跳转）

5. **栈操作**
   - `PUSH reg`：寄存器压栈
   - `POP reg`：寄存器出栈

6. **系统指令**
   - `HLT`：停止 CPU

7. **内存系统**
   - 16 MB 平坦内存
   - 边界检查
   - 8/16/32 位读写支持

---

## 🎯 代码功能说明

### CPU 模块 (`src/cpu.c`)
- **作用**：模拟 CPU 的核心执行流程
- **关键函数**：
  - `cpu_init()`：初始化 CPU，将所有寄存器清零
  - `cpu_reset()`：重置 CPU，设置 EIP 和 ESP
  - `cpu_step()`：执行一条指令（取指→译码→执行）
  - `cpu_run()`：运行多步指令，直到停止或达到最大步数

### 解码器模块 (`src/decoder.c`)
- **作用**：将内存中的二进制指令翻译成人类可理解的格式
- **示例**：
  - 内存：`0x10 0x00 0x2A 0x00 0x00 0x00`
  - 解码：`MOV EAX, 42`

### 执行模块 (`src/exec.c`)
- **作用**：实际执行指令，修改寄存器和内存
- **功能**：实现所有指令的执行逻辑

### 内存模块 (`src/memory.c`)
- **作用**：模拟 16 MB 的内存空间
- **功能**：提供内存读写接口，并进行边界检查

---

## 🔍 如何判断测试是否通过？

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

## 📝 下一步建议

当前项目完成度约 **30%**，建议按以下顺序实现：

1. **扩展基础指令集**（优先级：⭐⭐⭐⭐⭐）
   - AND, OR, XOR（按位运算）
   - NOT, INC, DEC（单操作数）
   - 移位指令（SHL, SHR）

2. **完善标志位**（优先级：⭐⭐⭐⭐）
   - CF（进位标志）
   - SF（符号标志）
   - OF（溢出标志）
   - PF（奇偶标志）

3. **添加内存访问指令**（优先级：⭐⭐⭐）
   - `MOV reg, [mem]`
   - `MOV [mem], reg`

4. **实现 CALL/RET**（优先级：⭐⭐⭐）
   - 支持子程序调用

5. **中断与异常**（优先级：⭐⭐）
   - 中断控制器
   - 异常处理

---

## 💡 学习建议

1. **从简单测试开始**：先运行 `test_simple`，理解基本概念
2. **观察详细执行**：运行 `test_detailed`，看每一步的变化
3. **修改测试程序**：尝试修改 `test_simple.c`，创建自己的测试
4. **阅读源代码**：从 `src/main.c` 开始，逐步理解各个模块

---

## 🚗 故障排除

### 编译失败
```bash
# 清理并重新编译
rm -rf build
cmake -S . -B build
cmake --build build -j
```

### 运行失败
- 检查是否正确编译：`ls build/` 确认可执行文件存在
- 查看错误信息：程序会输出详细的错误描述

### 测试失败
- 运行详细测试：`./build/test_detailed` 查看每一步执行情况
- 检查预期值是否合理
- 查看源代码中的断言（assert）语句

---

祝学习愉快！🎉
