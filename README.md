# myCPU

从 0 实现一个可运行程序的 x86 子集指令集模拟器（课程项目骨架）。

## 快速开始

```bash
cmake -S . -B build
cmake --build build -j
./build/mycpu
```

## 目录结构

- `include/mycpu/`: 模块头文件
- `src/`: 模块实现
- `tests/programs/`: 汇编测试程序
- `tools/`: 辅助脚本
- `docs/`: 设计文档

## 当前状态

- 已完成项目骨架与最小可运行入口
- 后续按模块实现 CPU/内存/指令/中断/外设
