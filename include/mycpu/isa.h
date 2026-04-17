#ifndef MYCPU_ISA_H
#define MYCPU_ISA_H

#include "mycpu/types.h"

typedef enum {
    REG_EAX = 0,
    REG_EBX,
    REG_ECX,
    REG_EDX,
    REG_ESI,
    REG_EDI,
    REG_EBP,
    REG_ESP,
    REG_COUNT
} reg_id_t;

typedef enum {
    // 数据传送指令 (0x10-0x1F)
    OP_MOV_REG_IMM = 0x10,
    OP_MOV_REG_REG = 0x11,
    OP_MOV_REG_MEM = 0x12,   // MOV reg, [reg] - 从内存读取
    OP_MOV_MEM_REG = 0x13,   // MOV [reg], reg - 写入内存
    
    // 算术运算指令 (0x20-0x2F)
    OP_ADD_REG_REG = 0x20,
    OP_SUB_REG_REG = 0x21,
    OP_CMP_REG_REG = 0x22,
    OP_ADD_REG_IMM = 0x23,
    OP_SUB_REG_IMM = 0x24,
    OP_INC_REG = 0x25,
    OP_DEC_REG = 0x26,
    OP_CMP_REG_IMM = 0x27,  // CMP reg, imm32
    
    // 逻辑运算指令 (0x28-0x2F)
    OP_AND_REG_REG = 0x28,
    OP_OR_REG_REG = 0x29,
    OP_XOR_REG_REG = 0x2A,
    OP_NOT_REG = 0x2B,
    OP_TEST_REG_REG = 0x2C,
    OP_AND_REG_IMM = 0x2D,
    OP_OR_REG_IMM = 0x2E,
    OP_XOR_REG_IMM = 0x2F,
    
    // 移位指令 (0x38-0x3F)
    OP_SHL_REG_REG = 0x38,
    OP_SHR_REG_REG = 0x39,
    
    // 杂项指令 (0x3A-0x3F)
    OP_NEG_REG = 0x3A,      // NEG reg - 取负
    OP_NOP = 0x3B,          // NOP - 空操作
    OP_CLC = 0x3C,          // CLC - 清除进位标志
    OP_STC = 0x3D,          // STC - 设置进位标志
    OP_LOOP_REL8 = 0x3E,    // LOOP rel8 - 循环指令
    
    // 控制流指令 (0x40-0x4F)
    OP_JMP_REL32 = 0x40,
    OP_JZ_REL32 = 0x41,   // Jump if Zero (ZF=1)
    OP_JNZ_REL32 = 0x42,  // Jump if Not Zero (ZF=0)
    OP_JC_REL32 = 0x43,   // Jump if Carry (CF=1)
    OP_JNC_REL32 = 0x44,  // Jump if Not Carry (CF=0)
    OP_JS_REL32 = 0x45,   // Jump if Sign (SF=1)
    OP_JNS_REL32 = 0x46,  // Jump if Not Sign (SF=0)
    OP_JO_REL32 = 0x47,   // Jump if Overflow (OF=1)
    OP_JNO_REL32 = 0x48,  // Jump if Not Overflow (OF=0)
    OP_CALL_REL32 = 0x49,
    OP_RET = 0x4A,
    
    // 栈操作指令 (0x50-0x5F)
    OP_PUSH_REG = 0x50,
    OP_POP_REG = 0x51,
    
    // 系统指令 (0xFF)
    OP_HLT = 0xFF
} opcode_t;

// 标志位定义 (x86 EFLAGS)
#define FLAG_CF (1u << 0)   // Carry Flag - 进位标志
#define FLAG_PF (1u << 2)   // Parity Flag - 奇偶标志
#define FLAG_AF (1u << 4)   // Auxiliary Carry Flag - 辅助进位标志
#define FLAG_ZF (1u << 6)   // Zero Flag - 零标志
#define FLAG_SF (1u << 7)   // Sign Flag - 符号标志
#define FLAG_OF (1u << 11)  // Overflow Flag - 溢出标志

#endif