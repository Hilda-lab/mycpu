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
    OP_MOV_REG_IMM = 0x10,
    OP_MOV_REG_REG = 0x11,
    OP_ADD_REG_REG = 0x20,
    OP_SUB_REG_REG = 0x21,
    OP_CMP_REG_REG = 0x22,
    OP_JMP_REL32 = 0x30,
    OP_JZ_REL32 = 0x31,
    OP_JNZ_REL32 = 0x32,
    OP_PUSH_REG = 0x40,
    OP_POP_REG = 0x41,
    OP_HLT = 0xFF
} opcode_t;

#define FLAG_ZF (1u << 6)

#endif