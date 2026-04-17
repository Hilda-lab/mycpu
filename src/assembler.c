#define _GNU_SOURCE
#include "mycpu/assembler.h"
#include "mycpu/isa.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  // for strcasecmp

#define MAX_LINE_LEN 256
#define MAX_LABELS 256
#define MAX_CODE_SIZE 65536

typedef struct {
    char name[64];
    uint32_t address;
} label_t;

typedef struct {
    label_t labels[MAX_LABELS];
    int label_count;
    uint8_t code[MAX_CODE_SIZE];
    size_t code_size;
    char error_msg[256];
    int error_line;
} assembler_t;

// Register name lookup
static int find_register(const char *name) {
    static const char *reg_names[] = {
        "EAX", "EBX", "ECX", "EDX", "ESI", "EDI", "EBP", "ESP",
        "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp",
        "ax", "bx", "cx", "dx", "si", "di", "bp", "sp"
    };
    
    for (int i = 0; i < 8; i++) {
        if (strcasecmp(name, reg_names[i]) == 0 ||
            strcasecmp(name, reg_names[i + 8]) == 0 ||
            strcasecmp(name, reg_names[i + 16]) == 0) {
            return i;
        }
    }
    return -1;
}

// Parse immediate value
static bool parse_imm(const char *str, uint32_t *value) {
    char *end;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        *value = (uint32_t)strtoul(str, &end, 16);
    } else if (str[0] == '\'') {
        // Character literal
        if (str[1] && str[2] == '\'') {
            *value = (uint8_t)str[1];
            return true;
        }
        return false;
    } else {
        *value = (uint32_t)strtoul(str, &end, 10);
    }
    return *end == '\0';
}

// Emit byte
static void emit_byte(assembler_t *a, uint8_t b) {
    if (a->code_size < MAX_CODE_SIZE) {
        a->code[a->code_size++] = b;
    }
}

// Emit word (16-bit, little-endian)
static void emit_word(assembler_t *a, uint16_t w) {
    emit_byte(a, w & 0xFF);
    emit_byte(a, (w >> 8) & 0xFF);
}

// Emit dword (32-bit, little-endian)
static void emit_dword(assembler_t *a, uint32_t d) {
    emit_byte(a, d & 0xFF);
    emit_byte(a, (d >> 8) & 0xFF);
    emit_byte(a, (d >> 16) & 0xFF);
    emit_byte(a, (d >> 24) & 0xFF);
}

// Add label
static bool add_label(assembler_t *a, const char *name, uint32_t addr) {
    if (a->label_count >= MAX_LABELS) {
        return false;
    }
    strncpy(a->labels[a->label_count].name, name, sizeof(a->labels[0].name) - 1);
    a->labels[a->label_count].address = addr;
    a->label_count++;
    return true;
}

// Find label
static int find_label(assembler_t *a, const char *name) {
    for (int i = 0; i < a->label_count; i++) {
        if (strcmp(a->labels[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Set error
static void set_error(assembler_t *a, int line, const char *msg) {
    a->error_line = line;
    strncpy(a->error_msg, msg, sizeof(a->error_msg) - 1);
}

// Parse operand and return type: 0=none, 1=reg, 2=imm, 3=mem, 4=label
static int parse_operand(char *op, int *reg, uint32_t *imm, char *label_name) {
    // Trim whitespace
    while (*op && isspace(*op)) op++;
    char *end = op + strlen(op) - 1;
    while (end > op && isspace(*end)) *end-- = '\0';
    
    if (*op == '\0') return 0;
    
    // Memory operand [reg]
    if (*op == '[') {
        char *close = strchr(op, ']');
        if (!close) return -1;
        *close = '\0';
        int r = find_register(op + 1);
        if (r < 0) return -1;
        *reg = r;
        return 3;
    }
    
    // Register
    int r = find_register(op);
    if (r >= 0) {
        *reg = r;
        return 1;
    }
    
    // Immediate (hex or decimal)
    if (isdigit(*op) || *op == '-' || *op == '+') {
        if (parse_imm(op, imm)) {
            return 2;
        }
    }
    
    // Label
    strncpy(label_name, op, 63);
    label_name[63] = '\0';
    return 4;
}

// First pass: collect labels
static bool first_pass(assembler_t *a, char *source) {
    char line[MAX_LINE_LEN];
    int line_num = 0;
    char *saveptr;
    char *token = strtok_r(source, "\n", &saveptr);
    
    while (token != NULL) {
        line_num++;
        strncpy(line, token, MAX_LINE_LEN - 1);
        line[MAX_LINE_LEN - 1] = '\0';
        
        // Remove comments
        char *comment = strchr(line, ';');
        if (comment) *comment = '\0';
        comment = strchr(line, '#');
        if (comment) *comment = '\0';
        
        // Trim leading whitespace
        char *p = line;
        while (*p && isspace(*p)) p++;
        
        // Empty line
        if (*p == '\0') {
            token = strtok_r(NULL, "\n", &saveptr);
            continue;
        }
        
        // Check for label
        char *colon = strchr(p, ':');
        if (colon) {
            *colon = '\0';
            if (!add_label(a, p, (uint32_t)a->code_size)) {
                set_error(a, line_num, "Too many labels");
                return false;
            }
            p = colon + 1;
            while (*p && isspace(*p)) p++;
        }
        
        // Empty after label
        if (*p == '\0') {
            token = strtok_r(NULL, "\n", &saveptr);
            continue;
        }
        
        // Parse instruction to get size
        char mnemonic[16] = {0};
        int i = 0;
        while (*p && !isspace(*p) && i < 15) {
            mnemonic[i++] = toupper(*p++);
        }
        mnemonic[i] = '\0';
        
        // Calculate instruction size
        size_t inst_size = 0;
        if (strcasecmp(mnemonic, "MOV") == 0) {
            // Need to peek at operands
            while (*p && isspace(*p)) p++;
            char *comma = strchr(p, ',');
            if (comma) {
                *comma = '\0';
                char op1[64], op2[64];
                strncpy(op1, p, 63);
                strncpy(op2, comma + 1, 63);
                int r1, r2, type1, type2;
                uint32_t imm;
                char label[64];
                type1 = parse_operand(op1, &r1, &imm, label);
                type2 = parse_operand(op2, &r2, &imm, label);
                if (type1 == 1 && type2 == 2) inst_size = 6;  // MOV reg, imm
                else if (type1 == 1 && type2 == 1) inst_size = 3;  // MOV reg, reg
                else if (type1 == 1 && type2 == 3) inst_size = 3;  // MOV reg, [reg]
                else if (type1 == 3 && type2 == 1) inst_size = 3;  // MOV [reg], reg
            }
        }
        else if (strcasecmp(mnemonic, "ADD") == 0 || strcasecmp(mnemonic, "SUB") == 0 ||
                 strcasecmp(mnemonic, "CMP") == 0 || strcasecmp(mnemonic, "AND") == 0 ||
                 strcasecmp(mnemonic, "OR") == 0 || strcasecmp(mnemonic, "XOR") == 0) {
            while (*p && isspace(*p)) p++;
            char *comma = strchr(p, ',');
            if (comma) {
                *comma = '\0';
                char op1[64], op2[64];
                strncpy(op1, p, 63);
                strncpy(op2, comma + 1, 63);
                int r1, r2, type1, type2;
                uint32_t imm;
                char label[64];
                type1 = parse_operand(op1, &r1, &imm, label);
                type2 = parse_operand(op2, &r2, &imm, label);
                if (type1 == 1 && type2 == 1) inst_size = 3;  // OP reg, reg
                else if (type1 == 1 && type2 == 2) inst_size = 6;  // OP reg, imm
            }
        }
        else if (strcasecmp(mnemonic, "INC") == 0 || strcasecmp(mnemonic, "DEC") == 0 ||
                 strcasecmp(mnemonic, "NOT") == 0 || strcasecmp(mnemonic, "NEG") == 0 ||
                 strcasecmp(mnemonic, "PUSH") == 0 || strcasecmp(mnemonic, "POP") == 0) {
            inst_size = 2;
        }
        else if (strcasecmp(mnemonic, "SHL") == 0 || strcasecmp(mnemonic, "SHR") == 0 ||
                 strcasecmp(mnemonic, "TEST") == 0) {
            inst_size = 3;
        }
        else if (strcasecmp(mnemonic, "JMP") == 0 || strcasecmp(mnemonic, "JZ") == 0 ||
                 strcasecmp(mnemonic, "JNZ") == 0 || strcasecmp(mnemonic, "JC") == 0 ||
                 strcasecmp(mnemonic, "JNC") == 0 || strcasecmp(mnemonic, "JS") == 0 ||
                 strcasecmp(mnemonic, "JNS") == 0 || strcasecmp(mnemonic, "JO") == 0 ||
                 strcasecmp(mnemonic, "JNO") == 0 || strcasecmp(mnemonic, "CALL") == 0) {
            inst_size = 5;
        }
        else if (strcasecmp(mnemonic, "LOOP") == 0) {
            inst_size = 2;
        }
        else if (strcasecmp(mnemonic, "RET") == 0 || strcasecmp(mnemonic, "HLT") == 0 ||
                 strcasecmp(mnemonic, "NOP") == 0 || strcasecmp(mnemonic, "CLC") == 0 ||
                 strcasecmp(mnemonic, "STC") == 0) {
            inst_size = 1;
        }
        else {
            set_error(a, line_num, "Unknown instruction");
            return false;
        }
        
        a->code_size += inst_size;
        token = strtok_r(NULL, "\n", &saveptr);
    }
    
    return true;
}

// Second pass: generate code
static bool second_pass(assembler_t *a, char *source) {
    char line[MAX_LINE_LEN];
    int line_num = 0;
    char *saveptr;
    a->code_size = 0;  // Reset for second pass
    
    char *token = strtok_r(source, "\n", &saveptr);
    
    while (token != NULL) {
        line_num++;
        strncpy(line, token, MAX_LINE_LEN - 1);
        line[MAX_LINE_LEN - 1] = '\0';
        
        // Remove comments
        char *comment = strchr(line, ';');
        if (comment) *comment = '\0';
        comment = strchr(line, '#');
        if (comment) *comment = '\0';
        
        // Trim leading whitespace
        char *p = line;
        while (*p && isspace(*p)) p++;
        
        // Skip label
        char *colon = strchr(p, ':');
        if (colon) {
            p = colon + 1;
            while (*p && isspace(*p)) p++;
        }
        
        // Empty after label
        if (*p == '\0') {
            token = strtok_r(NULL, "\n", &saveptr);
            continue;
        }
        
        // Parse instruction
        char mnemonic[16] = {0};
        int i = 0;
        while (*p && !isspace(*p) && i < 15) {
            mnemonic[i++] = toupper(*p++);
        }
        mnemonic[i] = '\0';
        
        // Parse operands
        while (*p && isspace(*p)) p++;
        char operands[128] = {0};
        strncpy(operands, p, 127);
        
        // Parse operand string
        char op1[64] = {0}, op2[64] = {0};
        char *comma = strchr(operands, ',');
        if (comma) {
            *comma = '\0';
            strncpy(op1, operands, 63);
            strncpy(op2, comma + 1, 63);
        } else {
            strncpy(op1, operands, 63);
        }
        
        int reg1, reg2, type1, type2;
        uint32_t imm = 0;
        char label[64] = {0};
        type1 = parse_operand(op1, &reg1, &imm, label);
        type2 = parse_operand(op2, &reg2, &imm, label);
        
        // Generate code
        if (strcasecmp(mnemonic, "MOV") == 0) {
            if (type1 == 1 && type2 == 2) {
                emit_byte(a, OP_MOV_REG_IMM);
                emit_byte(a, reg1);
                emit_dword(a, imm);
            }
            else if (type1 == 1 && type2 == 1) {
                emit_byte(a, OP_MOV_REG_REG);
                emit_byte(a, reg1);
                emit_byte(a, reg2);
            }
            else if (type1 == 1 && type2 == 3) {
                emit_byte(a, OP_MOV_REG_MEM);
                emit_byte(a, reg1);
                emit_byte(a, reg2);
            }
            else if (type1 == 3 && type2 == 1) {
                emit_byte(a, OP_MOV_MEM_REG);
                emit_byte(a, reg1);
                emit_byte(a, reg2);
            }
            else {
                set_error(a, line_num, "Invalid MOV operands");
                return false;
            }
        }
        else if (strcasecmp(mnemonic, "ADD") == 0) {
            if (type1 == 1 && type2 == 1) {
                emit_byte(a, OP_ADD_REG_REG);
                emit_byte(a, reg1);
                emit_byte(a, reg2);
            }
            else if (type1 == 1 && type2 == 2) {
                emit_byte(a, OP_ADD_REG_IMM);
                emit_byte(a, reg1);
                emit_dword(a, imm);
            }
            else {
                set_error(a, line_num, "Invalid ADD operands");
                return false;
            }
        }
        else if (strcasecmp(mnemonic, "SUB") == 0) {
            if (type1 == 1 && type2 == 1) {
                emit_byte(a, OP_SUB_REG_REG);
                emit_byte(a, reg1);
                emit_byte(a, reg2);
            }
            else if (type1 == 1 && type2 == 2) {
                emit_byte(a, OP_SUB_REG_IMM);
                emit_byte(a, reg1);
                emit_dword(a, imm);
            }
            else {
                set_error(a, line_num, "Invalid SUB operands");
                return false;
            }
        }
        else if (strcasecmp(mnemonic, "CMP") == 0) {
            if (type1 == 1 && type2 == 1) {
                emit_byte(a, OP_CMP_REG_REG);
                emit_byte(a, reg1);
                emit_byte(a, reg2);
            }
            else if (type1 == 1 && type2 == 2) {
                emit_byte(a, OP_CMP_REG_IMM);
                emit_byte(a, reg1);
                emit_dword(a, imm);
            }
            else {
                set_error(a, line_num, "Invalid CMP operands");
                return false;
            }
        }
        else if (strcasecmp(mnemonic, "AND") == 0) {
            if (type1 == 1 && type2 == 1) {
                emit_byte(a, OP_AND_REG_REG);
                emit_byte(a, reg1);
                emit_byte(a, reg2);
            }
            else if (type1 == 1 && type2 == 2) {
                emit_byte(a, OP_AND_REG_IMM);
                emit_byte(a, reg1);
                emit_dword(a, imm);
            }
            else {
                set_error(a, line_num, "Invalid AND operands");
                return false;
            }
        }
        else if (strcasecmp(mnemonic, "OR") == 0) {
            if (type1 == 1 && type2 == 1) {
                emit_byte(a, OP_OR_REG_REG);
                emit_byte(a, reg1);
                emit_byte(a, reg2);
            }
            else if (type1 == 1 && type2 == 2) {
                emit_byte(a, OP_OR_REG_IMM);
                emit_byte(a, reg1);
                emit_dword(a, imm);
            }
            else {
                set_error(a, line_num, "Invalid OR operands");
                return false;
            }
        }
        else if (strcasecmp(mnemonic, "XOR") == 0) {
            if (type1 == 1 && type2 == 1) {
                emit_byte(a, OP_XOR_REG_REG);
                emit_byte(a, reg1);
                emit_byte(a, reg2);
            }
            else if (type1 == 1 && type2 == 2) {
                emit_byte(a, OP_XOR_REG_IMM);
                emit_byte(a, reg1);
                emit_dword(a, imm);
            }
            else {
                set_error(a, line_num, "Invalid XOR operands");
                return false;
            }
        }
        else if (strcasecmp(mnemonic, "INC") == 0) {
            if (type1 == 1) {
                emit_byte(a, OP_INC_REG);
                emit_byte(a, reg1);
            }
            else {
                set_error(a, line_num, "Invalid INC operand");
                return false;
            }
        }
        else if (strcasecmp(mnemonic, "DEC") == 0) {
            if (type1 == 1) {
                emit_byte(a, OP_DEC_REG);
                emit_byte(a, reg1);
            }
            else {
                set_error(a, line_num, "Invalid DEC operand");
                return false;
            }
        }
        else if (strcasecmp(mnemonic, "NOT") == 0) {
            if (type1 == 1) {
                emit_byte(a, OP_NOT_REG);
                emit_byte(a, reg1);
            }
            else {
                set_error(a, line_num, "Invalid NOT operand");
                return false;
            }
        }
        else if (strcasecmp(mnemonic, "NEG") == 0) {
            if (type1 == 1) {
                emit_byte(a, OP_NEG_REG);
                emit_byte(a, reg1);
            }
            else {
                set_error(a, line_num, "Invalid NEG operand");
                return false;
            }
        }
        else if (strcasecmp(mnemonic, "SHL") == 0) {
            if (type1 == 1 && type2 == 1) {
                emit_byte(a, OP_SHL_REG_REG);
                emit_byte(a, reg1);
                emit_byte(a, reg2);
            }
            else {
                set_error(a, line_num, "Invalid SHL operands");
                return false;
            }
        }
        else if (strcasecmp(mnemonic, "SHR") == 0) {
            if (type1 == 1 && type2 == 1) {
                emit_byte(a, OP_SHR_REG_REG);
                emit_byte(a, reg1);
                emit_byte(a, reg2);
            }
            else {
                set_error(a, line_num, "Invalid SHR operands");
                return false;
            }
        }
        else if (strcasecmp(mnemonic, "TEST") == 0) {
            if (type1 == 1 && type2 == 1) {
                emit_byte(a, OP_TEST_REG_REG);
                emit_byte(a, reg1);
                emit_byte(a, reg2);
            }
            else {
                set_error(a, line_num, "Invalid TEST operands");
                return false;
            }
        }
        else if (strcasecmp(mnemonic, "PUSH") == 0) {
            if (type1 == 1) {
                emit_byte(a, OP_PUSH_REG);
                emit_byte(a, reg1);
            }
            else {
                set_error(a, line_num, "Invalid PUSH operand");
                return false;
            }
        }
        else if (strcasecmp(mnemonic, "POP") == 0) {
            if (type1 == 1) {
                emit_byte(a, OP_POP_REG);
                emit_byte(a, reg1);
            }
            else {
                set_error(a, line_num, "Invalid POP operand");
                return false;
            }
        }
        else if (strcasecmp(mnemonic, "JMP") == 0) {
            emit_byte(a, OP_JMP_REL32);
            uint32_t target_addr;
            if (type1 == 4) {  // Label
                int label_idx = find_label(a, op1);
                if (label_idx < 0) {
                    set_error(a, line_num, "Undefined label");
                    return false;
                }
                target_addr = a->labels[label_idx].address;
            } else {
                target_addr = imm;
            }
            int32_t rel = (int32_t)target_addr - (int32_t)(a->code_size + 4);
            emit_dword(a, (uint32_t)rel);
        }
        else if (strcasecmp(mnemonic, "JZ") == 0) {
            emit_byte(a, OP_JZ_REL32);
            uint32_t target_addr;
            if (type1 == 4) {
                int label_idx = find_label(a, op1);
                if (label_idx < 0) {
                    set_error(a, line_num, "Undefined label");
                    return false;
                }
                target_addr = a->labels[label_idx].address;
            } else {
                target_addr = imm;
            }
            int32_t rel = (int32_t)target_addr - (int32_t)(a->code_size + 4);
            emit_dword(a, (uint32_t)rel);
        }
        else if (strcasecmp(mnemonic, "JNZ") == 0) {
            emit_byte(a, OP_JNZ_REL32);
            uint32_t target_addr;
            if (type1 == 4) {
                int label_idx = find_label(a, op1);
                if (label_idx < 0) {
                    set_error(a, line_num, "Undefined label");
                    return false;
                }
                target_addr = a->labels[label_idx].address;
            } else {
                target_addr = imm;
            }
            int32_t rel = (int32_t)target_addr - (int32_t)(a->code_size + 4);
            emit_dword(a, (uint32_t)rel);
        }
        else if (strcasecmp(mnemonic, "JC") == 0) {
            emit_byte(a, OP_JC_REL32);
            uint32_t target_addr;
            if (type1 == 4) {
                int label_idx = find_label(a, op1);
                if (label_idx < 0) {
                    set_error(a, line_num, "Undefined label");
                    return false;
                }
                target_addr = a->labels[label_idx].address;
            } else {
                target_addr = imm;
            }
            int32_t rel = (int32_t)target_addr - (int32_t)(a->code_size + 4);
            emit_dword(a, (uint32_t)rel);
        }
        else if (strcasecmp(mnemonic, "JNC") == 0) {
            emit_byte(a, OP_JNC_REL32);
            uint32_t target_addr;
            if (type1 == 4) {
                int label_idx = find_label(a, op1);
                if (label_idx < 0) {
                    set_error(a, line_num, "Undefined label");
                    return false;
                }
                target_addr = a->labels[label_idx].address;
            } else {
                target_addr = imm;
            }
            int32_t rel = (int32_t)target_addr - (int32_t)(a->code_size + 4);
            emit_dword(a, (uint32_t)rel);
        }
        else if (strcasecmp(mnemonic, "JS") == 0) {
            emit_byte(a, OP_JS_REL32);
            uint32_t target_addr;
            if (type1 == 4) {
                int label_idx = find_label(a, op1);
                if (label_idx < 0) {
                    set_error(a, line_num, "Undefined label");
                    return false;
                }
                target_addr = a->labels[label_idx].address;
            } else {
                target_addr = imm;
            }
            int32_t rel = (int32_t)target_addr - (int32_t)(a->code_size + 4);
            emit_dword(a, (uint32_t)rel);
        }
        else if (strcasecmp(mnemonic, "JNS") == 0) {
            emit_byte(a, OP_JNS_REL32);
            uint32_t target_addr;
            if (type1 == 4) {
                int label_idx = find_label(a, op1);
                if (label_idx < 0) {
                    set_error(a, line_num, "Undefined label");
                    return false;
                }
                target_addr = a->labels[label_idx].address;
            } else {
                target_addr = imm;
            }
            int32_t rel = (int32_t)target_addr - (int32_t)(a->code_size + 4);
            emit_dword(a, (uint32_t)rel);
        }
        else if (strcasecmp(mnemonic, "JO") == 0) {
            emit_byte(a, OP_JO_REL32);
            uint32_t target_addr;
            if (type1 == 4) {
                int label_idx = find_label(a, op1);
                if (label_idx < 0) {
                    set_error(a, line_num, "Undefined label");
                    return false;
                }
                target_addr = a->labels[label_idx].address;
            } else {
                target_addr = imm;
            }
            int32_t rel = (int32_t)target_addr - (int32_t)(a->code_size + 4);
            emit_dword(a, (uint32_t)rel);
        }
        else if (strcasecmp(mnemonic, "JNO") == 0) {
            emit_byte(a, OP_JNO_REL32);
            uint32_t target_addr;
            if (type1 == 4) {
                int label_idx = find_label(a, op1);
                if (label_idx < 0) {
                    set_error(a, line_num, "Undefined label");
                    return false;
                }
                target_addr = a->labels[label_idx].address;
            } else {
                target_addr = imm;
            }
            int32_t rel = (int32_t)target_addr - (int32_t)(a->code_size + 4);
            emit_dword(a, (uint32_t)rel);
        }
        else if (strcasecmp(mnemonic, "LOOP") == 0) {
            emit_byte(a, OP_LOOP_REL8);
            uint32_t target_addr;
            if (type1 == 4) {
                int label_idx = find_label(a, op1);
                if (label_idx < 0) {
                    set_error(a, line_num, "Undefined label");
                    return false;
                }
                target_addr = a->labels[label_idx].address;
            } else {
                target_addr = imm;
            }
            int32_t rel = (int32_t)target_addr - (int32_t)(a->code_size + 1);
            if (rel < -128 || rel > 127) {
                set_error(a, line_num, "LOOP target out of range (-128 to 127)");
                return false;
            }
            emit_byte(a, (uint8_t)(rel & 0xFF));
        }
        else if (strcasecmp(mnemonic, "CALL") == 0) {
            emit_byte(a, OP_CALL_REL32);
            uint32_t target_addr;
            if (type1 == 4) {
                int label_idx = find_label(a, op1);
                if (label_idx < 0) {
                    set_error(a, line_num, "Undefined label");
                    return false;
                }
                target_addr = a->labels[label_idx].address;
            } else {
                target_addr = imm;
            }
            int32_t rel = (int32_t)target_addr - (int32_t)(a->code_size + 4);
            emit_dword(a, (uint32_t)rel);
        }
        else if (strcasecmp(mnemonic, "RET") == 0) {
            emit_byte(a, OP_RET);
        }
        else if (strcasecmp(mnemonic, "HLT") == 0) {
            emit_byte(a, OP_HLT);
        }
        else if (strcasecmp(mnemonic, "NOP") == 0) {
            emit_byte(a, OP_NOP);
        }
        else if (strcasecmp(mnemonic, "CLC") == 0) {
            emit_byte(a, OP_CLC);
        }
        else if (strcasecmp(mnemonic, "STC") == 0) {
            emit_byte(a, OP_STC);
        }
        
        token = strtok_r(NULL, "\n", &saveptr);
    }
    
    return true;
}

asm_result_t assemble(const char *source) {
    asm_result_t result = {0};
    assembler_t assembler = {0};
    
    // Make a copy for each pass
    char *source1 = strdup(source);
    char *source2 = strdup(source);
    
    if (!source1 || !source2) {
        result.error_msg = strdup("Memory allocation failed");
        free(source1);
        free(source2);
        return result;
    }
    
    // First pass: collect labels
    if (!first_pass(&assembler, source1)) {
        result.error_msg = strdup(assembler.error_msg);
        result.error_line = assembler.error_line;
        free(source1);
        free(source2);
        return result;
    }
    
    // Second pass: generate code
    if (!second_pass(&assembler, source2)) {
        result.error_msg = strdup(assembler.error_msg);
        result.error_line = assembler.error_line;
        free(source1);
        free(source2);
        return result;
    }
    
    // Copy result
    result.code = malloc(assembler.code_size);
    if (result.code) {
        memcpy(result.code, assembler.code, assembler.code_size);
        result.size = assembler.code_size;
    }
    
    free(source1);
    free(source2);
    return result;
}

asm_result_t assemble_file(const char *filename) {
    asm_result_t result = {0};
    
    FILE *f = fopen(filename, "r");
    if (!f) {
        result.error_msg = strdup("Cannot open file");
        return result;
    }
    
    // Read file
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *source = malloc(size + 1);
    if (!source) {
        result.error_msg = strdup("Memory allocation failed");
        fclose(f);
        return result;
    }
    
    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);
    
    result = assemble(source);
    free(source);
    return result;
}

void asm_result_free(asm_result_t *result) {
    if (result) {
        free(result->code);
        free(result->error_msg);
        result->code = NULL;
        result->error_msg = NULL;
        result->size = 0;
    }
}

bool write_binary(const char *filename, const uint8_t *code, size_t size) {
    FILE *f = fopen(filename, "wb");
    if (!f) return false;
    
    size_t written = fwrite(code, 1, size, f);
    fclose(f);
    return written == size;
}

void print_disassembly(const uint8_t *code, size_t size) {
    // Simple hex dump
    for (size_t i = 0; i < size; i++) {
        printf("%02X ", code[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if (size % 16 != 0) printf("\n");
}
