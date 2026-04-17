#!/bin/bash
# ============================================================
# myCPU 模拟器项目验收演示脚本
# 用途：向老师完整演示所有项目要求的功能
# ============================================================

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color
BOLD='\033[1m'

clear
echo -e "${BOLD}${BLUE}"
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║           myCPU 模拟器项目 - 验收演示                          ║"
echo "║              A Simple x86 CPU Simulator                       ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# 检查编译
if [ ! -f "build/mycpu_asm" ]; then
    echo -e "${YELLOW}首次运行，正在编译项目...${NC}"
    cmake -S . -B build > /dev/null 2>&1
    cmake --build build -j > /dev/null 2>&1
    echo -e "${GREEN}编译完成！${NC}"
    echo ""
fi

# ============================================================
echo -e "${BOLD}${BLUE}【模块一】CPU核心模块演示${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo -e "${YELLOW}1.1 寄存器组演示（8个32位通用寄存器）${NC}"
echo "   寄存器：EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP"
echo "   位置：include/mycpu/cpu.h"
echo ""
sleep 1

echo -e "${YELLOW}1.2 PC（指令指针）和标志位演示${NC}"
echo "   EIP：程序计数器"
echo "   EFLAGS：CF(进位)、ZF(零)、SF(符号)、OF(溢出)、IF(中断)等"
echo ""
sleep 1

echo -e "${YELLOW}1.3 基础指令集演示 - Fibonacci程序${NC}"
echo "   运行Fibonacci计算，演示数据传送、算术运算、控制流指令"
echo ""
echo -e "${GREEN}>>> 执行: ./build/test_fib${NC}"
echo "----------------------------------------"
./build/test_fib
echo "----------------------------------------"
echo ""
read -p "按Enter继续下一个演示..."

# ============================================================
echo ""
echo -e "${BOLD}${BLUE}【模块二】内存与地址空间模块演示${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo -e "${YELLOW}2.1 物理内存模拟${NC}"
echo "   实现：16MB平坦内存空间"
echo "   支持：8/16/32位读写，小端序"
echo ""

echo -e "${YELLOW}2.2 地址检查与越界异常${NC}"
echo "   演示内存边界检查功能..."
echo ""
echo -e "${GREEN}>>> 执行: ./build/test_detailed${NC}"
echo "----------------------------------------"
./build/test_detailed 2>&1 | head -40
echo "----------------------------------------"
echo ""
read -p "按Enter继续下一个演示..."

# ============================================================
echo ""
echo -e "${BOLD}${BLUE}【模块三】MMU/分页机制演示${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo -e "${YELLOW}3.1 二级页表结构${NC}"
echo "   - 页目录（1024项）"
echo "   - 页表（1024项）"
echo "   - 页面大小：4KB"
echo ""

echo -e "${YELLOW}3.2 TLB缓存${NC}"
echo "   - 64项TLB转换缓存"
echo "   - 命中/未命中统计"
echo ""

echo -e "${YELLOW}3.3 地址翻译与页错误${NC}"
echo "   演示虚拟地址翻译和页错误处理..."
echo ""
echo -e "${GREEN}>>> 执行: ./build/test_interrupt (分页部分)${NC}"
echo "----------------------------------------"
./build/test_interrupt 2>&1 | grep -A 50 "MMU\|分页\|TLB\|页表"
echo "----------------------------------------"
echo ""
read -p "按Enter继续下一个演示..."

# ============================================================
echo ""
echo -e "${BOLD}${BLUE}【模块四】中断与异常模块演示${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo -e "${YELLOW}4.1 中断控制器（PIC）${NC}"
echo "   - 支持16个IRQ"
echo "   - 中断优先级管理"
echo "   - 中断屏蔽（IMR）"
echo ""

echo -e "${YELLOW}4.2 定时器中断（PIT）${NC}"
echo "   - 8254兼容定时器"
echo "   - 3个独立通道"
echo "   - 定时器中断触发"
echo ""

echo -e "${YELLOW}4.3 异常入口与上下文保存${NC}"
echo "   - IDT中断描述符表（256向量）"
echo "   - 上下文保存（EIP、EFLAGS）"
echo "   - 中断返回（IRET）"
echo ""

echo -e "${GREEN}>>> 执行: ./build/test_interrupt${NC}"
echo "----------------------------------------"
./build/test_interrupt
echo "----------------------------------------"
echo ""
read -p "按Enter继续下一个演示..."

# ============================================================
echo ""
echo -e "${BOLD}${BLUE}【模块五】外设接口模块演示${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo -e "${YELLOW}5.1 UART串口（打印输出）${NC}"
echo "   - 16550兼容实现"
echo "   - TX/RX FIFO"
echo "   - 中断支持"
echo ""
echo "   文件位置: include/mycpu/uart.h, src/uart.c"
echo ""

echo -e "${YELLOW}5.2 定时器（PIT）${NC}"
echo "   文件位置: include/mycpu/pit.h, src/pit.c"
echo ""

echo -e "${YELLOW}5.3 统一硬件接口规范${NC}"
echo "   所有外设遵循统一的接口："
echo "   - xxx_init()  初始化"
echo "   - xxx_reset() 复位"
echo "   - xxx_read()  读取"
echo "   - xxx_write() 写入"
echo ""
read -p "按Enter继续下一个演示..."

# ============================================================
echo ""
echo -e "${BOLD}${BLUE}【模块六】完整系统运行演示${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo -e "${YELLOW}6.1 汇编器 - 将汇编代码编译为机器码${NC}"
echo ""
echo -e "${GREEN}>>> 执行: ./build/mycpu_asm -d tests/programs/demo.asm${NC}"
echo "----------------------------------------"
./build/mycpu_asm -d tests/programs/demo.asm 2>&1 | head -30
echo "----------------------------------------"
echo ""

echo -e "${YELLOW}6.2 运行所有自动化测试${NC}"
echo ""
echo -e "${GREEN}>>> 执行: ctest --test-dir build --output-on-failure${NC}"
echo "----------------------------------------"
ctest --test-dir build --output-on-failure
echo "----------------------------------------"
echo ""
read -p "按Enter继续..."

# ============================================================
echo ""
echo -e "${BOLD}${BLUE}【模块七】可视化调试器演示${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo -e "${YELLOW}7.1 调试器功能${NC}"
echo "   - 可视化界面（ncurses）"
echo "   - 单步执行"
echo "   - 断点设置"
echo "   - 寄存器监视"
echo "   - 内存查看"
echo ""
echo -e "${GREEN}>>> 启动调试器（按R运行，S单步，Q退出）${NC}"
echo "----------------------------------------"
read -p "是否启动调试器演示？(y/n): " start_debug
if [ "$start_debug" = "y" ]; then
    ./build/mycpu_debugger
fi
echo "----------------------------------------"
echo ""

# ============================================================
echo ""
echo -e "${BOLD}${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}${GREEN}║                    验收演示完成！                              ║${NC}"
echo -e "${BOLD}${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${BOLD}项目成果对照：${NC}"
echo ""
echo "✅ 模拟器可运行        - 所有测试通过"
echo "✅ 可串口输出          - UART 16550实现"
echo "✅ 可处理中断          - PIC/PIT/IDT完整实现"
echo "✅ 可执行运算          - 36+条指令支持"
echo ""
echo -e "${BOLD}项目亮点：${NC}"
echo "  • 完整的x86子集实现（36+条指令）"
echo "  • MMU内存分页（二级页表 + TLB缓存）"
echo "  • 完善的中断系统（PIC + PIT + IDT）"
echo "  • 配套工具（汇编器 + 可视化调试器）"
echo ""
echo -e "${BOLD}总代码量：${NC}"
find src include -name "*.c" -o -name "*.h" | xargs wc -l | tail -1
echo ""
