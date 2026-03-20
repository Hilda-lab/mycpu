# myCPU

从 0 实现一个可运行程序的 x86 子集指令集模拟器（课程项目骨架）。

## 快速开始

```bash
cmake -S . -B build
cmake --build build -j
./build/mycpu
ctest --test-dir build --output-on-failure
```

## 第一批已实现指令

- `MOV`：`reg <- imm32`、`reg <- reg`
- `ADD`：`reg += reg`
- `SUB`：`reg -= reg`
- `CMP`：更新 `ZF`
- `JMP`：相对跳转
- `JZ`：`ZF=1` 时跳转
- `JNZ`：`ZF=0` 时跳转
- `PUSH`：32 位寄存器压栈
- `POP`：32 位寄存器出栈
- `HLT`：停机

> 当前是教学阶段的“最小可运行指令编码”，用于快速验证 CPU 执行链路；后续可逐步替换为更贴近 x86 机器码的解码实现。

## 目录结构

- `include/mycpu/`: 模块头文件
- `src/`: 模块实现
- `tests/programs/`: 汇编测试程序
- `tools/`: 辅助脚本
- `docs/`: 设计文档

## 当前状态

- 已具备 CPU 主循环：取指、译码、执行
- 已支持第一批 10 条指令并提供自动化测试
- 后续按模块迭代中断、外设和更完整的 x86 解码
