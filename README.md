# myCPU

一个从零实现的 x86 子集指令集模拟器，支持可视化调试和汇编语言编译。

## 功能特性

- **CPU 核心**：8 个 32 位通用寄存器、标志寄存器、取指-译码-执行流水线
- **内存系统**：16 MB 平坦内存空间，支持 8/16/32 位读写
- **指令集**：已实现 30+ 条指令（数据传送、算术、逻辑、移位、控制流、栈操作）
- **可视化调试器**：ncurses 终端界面，支持单步执行、断点、内存查看、反汇编
- **汇编器**：支持汇编语言源码编译为机器码

## 快速开始

```bash
# 编译
cmake -S . -B build
cmake --build build -j

# 运行演示程序
./build/mycpu

# 运行可视化调试器
./build/mycpu_debugger

# 汇编并运行程序
./build/mycpu_asm -d -r tests/programs/demo.asm

# 运行所有测试
./run_all_tests.sh
```

## 可执行程序

| 程序 | 说明 |
|------|------|
| `mycpu` | 基础演示程序 |
| `mycpu_debugger` | 可视化调试器（ncurses 界面） |
| `mycpu_asm` | 汇编器（汇编语言 → 机器码） |
| `test_fib` | Fibonacci 计算测试 |
| `test_simple` | 基础指令测试 |
| `test_new_inst` | 新指令测试（NEG, NOP, CLC, STC, LOOP） |
| `test_detailed` | 详细执行过程演示 |
| `mycpu_tests` | 自动化单元测试 |

## 汇编器使用

```bash
# 基本用法
./build/mycpu_asm program.asm

# 指定输出文件
./build/mycpu_asm -o program.bin program.asm

# 显示生成的机器码
./build/mycpu_asm -d program.asm

# 汇编后立即运行
./build/mycpu_asm -r program.asm
```

**汇编语法示例：**
```asm
; Fibonacci 序列计算
    MOV EAX, 1
    MOV EBX, 1
    MOV ECX, 10
loop_start:
    CMP ECX, 0
    JZ done
    MOV EDX, EAX
    ADD EDX, EBX
    MOV EBX, EAX
    MOV EAX, EDX
    DEC ECX
    JMP loop_start
done:
    HLT
```

## 调试器使用

```bash
./build/mycpu_debugger
```

**快捷键**：
- `R` - 连续运行（按任意键暂停）
- `S` - 单步执行
- `B` - 在当前 EIP 设置断点
- `P` - 输入地址设置断点
- `M` - 内存视图跳转到 EIP
- `G` - 跳转到指定内存地址
- `J/K` - 内存视图向下/向上滚动
- `X` - 重置 CPU
- `Q` - 退出

## 已实现指令

### 数据传送
`MOV reg, imm32` | `MOV reg, reg` | `MOV reg, [reg]` | `MOV [reg], reg`

### 算术运算
`ADD reg, reg` | `ADD reg, imm32` | `SUB reg, reg` | `SUB reg, imm32` | `INC reg` | `DEC reg` | `NEG reg`

### 逻辑运算
`AND reg, reg` | `AND reg, imm32` | `OR reg, reg` | `OR reg, imm32` | `XOR reg, reg` | `XOR reg, imm32` | `NOT reg` | `TEST reg, reg`

### 移位操作
`SHL reg, reg` | `SHR reg, reg`

### 比较与测试
`CMP reg, reg` | `CMP reg, imm32`

### 控制流
`JMP rel32` | `JZ rel32` | `JNZ rel32` | `JC rel32` | `JNC rel32` | `JS rel32` | `JNS rel32` | `JO rel32` | `JNO rel32` | `CALL rel32` | `RET` | `LOOP rel8`

### 标志位操作
`CLC` | `STC`

### 其他
`NOP` | `HLT`

## 目录结构

```
mycpu/
├── include/mycpu/    # 头文件
│   ├── cpu.h         # CPU 结构和状态
│   ├── memory.h      # 内存接口
│   ├── isa.h         # 指令集定义
│   ├── decoder.h     # 指令解码器
│   ├── exec.h        # 指令执行
│   ├── debugger.h    # 调试器接口
│   ├── assembler.h   # 汇编器接口
│   └── ...
├── src/              # 源代码实现
├── tests/            # 测试程序
│   └── programs/     # 汇编测试程序
├── docs/             # 设计文档
└── tools/            # 辅助工具
```

## 测试

```bash
# 运行自动化测试
ctest --test-dir build --output-on-failure

# 运行 Fibonacci 测试（验证指令正确性）
./build/test_fib

# 运行新指令测试
./build/test_new_inst
```

## 详细文档

- `PROJECT_STATUS.md` - 项目状态与开发进度
- `DEBUGGER_GUIDE.txt` - 调试器使用指南
- `TESTING_GUIDE.md` - 测试指南
