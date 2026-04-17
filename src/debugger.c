#include "mycpu/debugger.h"
#include "mycpu/isa.h"
#include "mycpu/decoder.h"

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>

#define COLOR_REG_CHANGED 1
#define COLOR_FLAG_ON     2
#define COLOR_FLAG_OFF    3
#define COLOR_HIGHLIGHT   4
#define COLOR_MENU        5
#define COLOR_BORDER      6
#define COLOR_TITLE       7

// Register names
static const char *reg_names[] = {
    "EAX", "EBX", "ECX", "EDX", "ESI", "EDI", "EBP", "ESP"
};

// Last register values (for highlighting changes)
static uint32_t last_regs[8] = {0};
static uint32_t last_eflags = 0;

void debugger_init(debugger_t *dbg, cpu_t *cpu, memory_t *memory) {
    memset(dbg, 0, sizeof(*dbg));
    dbg->cpu = cpu;
    dbg->memory = memory;
    dbg->state = DBG_PAUSED;
    dbg->breakpoint_count = 0;
    dbg->step_count = 0;
    dbg->mem_view_addr = 0;
    dbg->selected_menu = 0;
}

bool debugger_add_breakpoint(debugger_t *dbg, uint32_t address) {
    if (dbg->breakpoint_count >= MAX_BREAKPOINTS) {
        return false;
    }
    for (int i = 0; i < dbg->breakpoint_count; i++) {
        if (dbg->breakpoints[i].address == address) {
            dbg->breakpoints[i].enabled = true;
            return true;
        }
    }
    dbg->breakpoints[dbg->breakpoint_count].address = address;
    dbg->breakpoints[dbg->breakpoint_count].enabled = true;
    dbg->breakpoint_count++;
    return true;
}

bool debugger_remove_breakpoint(debugger_t *dbg, uint32_t address) {
    for (int i = 0; i < dbg->breakpoint_count; i++) {
        if (dbg->breakpoints[i].address == address) {
            dbg->breakpoints[i].enabled = false;
            return true;
        }
    }
    return false;
}

bool debugger_check_breakpoint(debugger_t *dbg) {
    for (int i = 0; i < dbg->breakpoint_count; i++) {
        if (dbg->breakpoints[i].enabled && 
            dbg->breakpoints[i].address == dbg->cpu->eip) {
            return true;
        }
    }
    return false;
}

// Get opcode mnemonic
static const char *get_opcode_name(uint8_t opcode) {
    switch (opcode) {
        case OP_MOV_REG_IMM: return "MOV";
        case OP_MOV_REG_REG: return "MOV";
        case OP_MOV_REG_MEM: return "MOV";
        case OP_MOV_MEM_REG: return "MOV";
        case OP_ADD_REG_REG: return "ADD";
        case OP_ADD_REG_IMM: return "ADD";
        case OP_SUB_REG_REG: return "SUB";
        case OP_SUB_REG_IMM: return "SUB";
        case OP_CMP_REG_REG: return "CMP";
        case OP_CMP_REG_IMM: return "CMP";
        case OP_INC_REG:     return "INC";
        case OP_DEC_REG:     return "DEC";
        case OP_AND_REG_REG: return "AND";
        case OP_AND_REG_IMM: return "AND";
        case OP_OR_REG_REG:  return "OR";
        case OP_OR_REG_IMM:  return "OR";
        case OP_XOR_REG_REG: return "XOR";
        case OP_XOR_REG_IMM: return "XOR";
        case OP_NOT_REG:     return "NOT";
        case OP_TEST_REG_REG: return "TEST";
        case OP_SHL_REG_REG: return "SHL";
        case OP_SHR_REG_REG: return "SHR";
        case OP_NEG_REG:     return "NEG";
        case OP_NOP:         return "NOP";
        case OP_CLC:         return "CLC";
        case OP_STC:         return "STC";
        case OP_LOOP_REL8:   return "LOOP";
        case OP_JMP_REL32:   return "JMP";
        case OP_JZ_REL32:    return "JZ ";
        case OP_JNZ_REL32:   return "JNZ";
        case OP_JC_REL32:    return "JC ";
        case OP_JNC_REL32:   return "JNC";
        case OP_JS_REL32:    return "JS ";
        case OP_JNS_REL32:   return "JNS";
        case OP_JO_REL32:    return "JO ";
        case OP_JNO_REL32:   return "JNO";
        case OP_CALL_REL32:  return "CALL";
        case OP_RET:         return "RET";
        case OP_PUSH_REG:    return "PUSH";
        case OP_POP_REG:     return "POP";
        case OP_HLT:         return "HLT";
        default:             return "???";
    }
}

// Disassemble single instruction
static void disassemble(memory_t *mem, uint32_t addr, char *buf, size_t size) {
    instruction_t inst;
    if (!decode_instruction(mem, addr, &inst)) {
        snprintf(buf, size, "???");
        return;
    }
    
    const char *op = get_opcode_name(inst.opcode);
    
    switch (inst.opcode) {
        case OP_MOV_REG_IMM:
        case OP_ADD_REG_IMM:
        case OP_SUB_REG_IMM:
        case OP_CMP_REG_IMM:
        case OP_AND_REG_IMM:
        case OP_OR_REG_IMM:
        case OP_XOR_REG_IMM:
            snprintf(buf, size, "%s %s, 0x%08X", op, 
                     reg_names[inst.reg_a], inst.imm32);
            break;
            
        case OP_MOV_REG_REG:
        case OP_ADD_REG_REG:
        case OP_SUB_REG_REG:
        case OP_CMP_REG_REG:
        case OP_AND_REG_REG:
        case OP_OR_REG_REG:
        case OP_XOR_REG_REG:
        case OP_TEST_REG_REG:
        case OP_SHL_REG_REG:
        case OP_SHR_REG_REG:
            snprintf(buf, size, "%s %s, %s", op,
                     reg_names[inst.reg_a], reg_names[inst.reg_b]);
            break;
            
        case OP_MOV_REG_MEM:
            snprintf(buf, size, "%s %s, [%s]", op,
                     reg_names[inst.reg_a], reg_names[inst.reg_b]);
            break;
            
        case OP_MOV_MEM_REG:
            snprintf(buf, size, "%s [%s], %s", op,
                     reg_names[inst.reg_a], reg_names[inst.reg_b]);
            break;
            
        case OP_INC_REG:
        case OP_DEC_REG:
        case OP_NOT_REG:
        case OP_NEG_REG:
        case OP_PUSH_REG:
        case OP_POP_REG:
            snprintf(buf, size, "%s %s", op, reg_names[inst.reg_a]);
            break;
            
        case OP_NOP:
        case OP_CLC:
        case OP_STC:
        case OP_RET:
        case OP_HLT:
            snprintf(buf, size, "%s", op);
            break;
            
        case OP_LOOP_REL8: {
            int32_t rel = (int32_t)inst.imm32;
            uint32_t target = addr + inst.length + rel;
            snprintf(buf, size, "%s 0x%08X", op, target);
            break;
        }
            
        case OP_JMP_REL32:
        case OP_JZ_REL32:
        case OP_JNZ_REL32:
        case OP_JC_REL32:
        case OP_JNC_REL32:
        case OP_JS_REL32:
        case OP_JNS_REL32:
        case OP_JO_REL32:
        case OP_JNO_REL32:
        case OP_CALL_REL32: {
            int32_t rel = (int32_t)inst.imm32;
            uint32_t target = addr + inst.length + rel;
            snprintf(buf, size, "%s 0x%08X", op, target);
            break;
        }
        
        default:
            snprintf(buf, size, "??? (0x%02X)", inst.opcode);
            break;
    }
}

// Draw a box with title
static void draw_box(int y, int x, int h, int w, const char *title) {
    // Draw corners
    mvaddch(y, x, ACS_ULCORNER);
    mvaddch(y, x + w - 1, ACS_URCORNER);
    mvaddch(y + h - 1, x, ACS_LLCORNER);
    mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);
    
    // Draw horizontal lines
    for (int i = 1; i < w - 1; i++) {
        mvaddch(y, x + i, ACS_HLINE);
        mvaddch(y + h - 1, x + i, ACS_HLINE);
    }
    
    // Draw vertical lines
    for (int i = 1; i < h - 1; i++) {
        mvaddch(y + i, x, ACS_VLINE);
        mvaddch(y + i, x + w - 1, ACS_VLINE);
    }
    
    // Draw title
    if (title) {
        attron(COLOR_PAIR(COLOR_TITLE) | A_BOLD);
        mvprintw(y, x + 2, " %s ", title);
        attroff(COLOR_PAIR(COLOR_TITLE) | A_BOLD);
    }
}

// Draw register panel
static void draw_registers(debugger_t *dbg, int y, int x, int w) {
    draw_box(y, x, 11, w, "REGISTERS");
    
    cpu_t *cpu = dbg->cpu;
    uint32_t regs[] = {cpu->eax, cpu->ebx, cpu->ecx, cpu->edx,
                       cpu->esi, cpu->edi, cpu->ebp, cpu->esp};
    
    for (int i = 0; i < 8; i++) {
        int row = y + 1 + i;
        bool changed = (regs[i] != last_regs[i]);
        
        if (changed) {
            attron(COLOR_PAIR(COLOR_REG_CHANGED) | A_BOLD);
        }
        mvprintw(row, x + 2, "%s: 0x%08X", reg_names[i], regs[i]);
        if (changed) {
            attroff(COLOR_PAIR(COLOR_REG_CHANGED) | A_BOLD);
        }
    }
    
    memcpy(last_regs, regs, sizeof(regs));
}

// Draw flags panel
static void draw_flags(debugger_t *dbg, int y, int x, int w) {
    draw_box(y, x, 5, w, "FLAGS");
    
    cpu_t *cpu = dbg->cpu;
    uint32_t flags = cpu->eflags;
    bool changed = (flags != last_eflags);
    
    if (changed) {
        attron(COLOR_PAIR(COLOR_REG_CHANGED));
    }
    mvprintw(y + 1, x + 2, "EFLAGS: 0x%08X", flags);
    if (changed) {
        attroff(COLOR_PAIR(COLOR_REG_CHANGED));
    }
    
    struct { const char *name; uint32_t bit; const char *desc; } flag_list[] = {
        {"CF", FLAG_CF, "Carry"},    {"PF", FLAG_PF, "Parity"},
        {"AF", FLAG_AF, "Aux"},      {"ZF", FLAG_ZF, "Zero"},
        {"SF", FLAG_SF, "Sign"},     {"OF", FLAG_OF, "Overflow"}
    };
    
    mvprintw(y + 2, x + 2, "  ");
    for (int i = 0; i < 6; i++) {
        bool is_set = (flags & flag_list[i].bit) != 0;
        if (is_set) {
            attron(COLOR_PAIR(COLOR_FLAG_ON) | A_BOLD);
            printw("%s=1 ", flag_list[i].name);
            attroff(COLOR_PAIR(COLOR_FLAG_ON) | A_BOLD);
        } else {
            attron(COLOR_PAIR(COLOR_FLAG_OFF));
            printw("%s=0 ", flag_list[i].name);
            attroff(COLOR_PAIR(COLOR_FLAG_OFF));
        }
    }
    
    mvprintw(y + 3, x + 2, "Last Changed Flag: ");
    for (int i = 0; i < 6; i++) {
        bool now = (flags & flag_list[i].bit) != 0;
        bool old = (last_eflags & flag_list[i].bit) != 0;
        if (now != old) {
            printw("%s ", flag_list[i].name);
        }
    }
    
    last_eflags = flags;
}

// Draw memory panel
static void draw_memory(debugger_t *dbg, int y, int x, int w) {
    draw_box(y, x, 11, w, "MEMORY DUMP");
    
    mvprintw(y + 1, x + 2, "Base: 0x%08X", dbg->mem_view_addr);
    mvprintw(y + 2, x + 2, "[G/Goto] [J/Down] [K/Up]");
    
    for (int row = 0; row < 7; row++) {
        uint32_t addr = dbg->mem_view_addr + row * 16;
        mvprintw(y + 3 + row, x + 2, "%08X: ", addr);
        
        for (int col = 0; col < 16; col++) {
            uint8_t val;
            if (memory_read8(dbg->memory, addr + col, &val)) {
                printw("%02X ", val);
            } else {
                printw("?? ");
            }
        }
    }
}

// Draw disassembly panel
static void draw_disassembly(debugger_t *dbg, int y, int x, int w, int lines) {
    draw_box(y, x, lines + 2, w, "DISASSEMBLY");
    
    uint32_t addr = dbg->cpu->eip;
    char buf[64];
    
    for (int i = 0; i < lines && addr < dbg->memory->size; i++) {
        instruction_t inst;
        if (!decode_instruction(dbg->memory, addr, &inst)) {
            break;
        }
        
        disassemble(dbg->memory, addr, buf, sizeof(buf));
        
        // Highlight current instruction
        if (addr == dbg->cpu->eip) {
            attron(COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
            mvprintw(y + 1 + i, x + 2, ">>> %08X: %-30s", addr, buf);
            attroff(COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
        } else {
            mvprintw(y + 1 + i, x + 2, "    %08X: %-30s", addr, buf);
        }
        
        addr += inst.length;
    }
}

// Draw breakpoints panel
static void draw_breakpoints(debugger_t *dbg, int y, int x, int w) {
    draw_box(y, x, 8, w, "BREAKPOINTS");
    
    if (dbg->breakpoint_count == 0) {
        mvprintw(y + 1, x + 2, "No breakpoints");
        mvprintw(y + 2, x + 2, "Press 'B' to set at EIP");
    } else {
        for (int i = 0; i < dbg->breakpoint_count && i < 6; i++) {
            if (dbg->breakpoints[i].enabled) {
                mvprintw(y + 1 + i, x + 2, "[%d] 0x%08X %s", i, 
                         dbg->breakpoints[i].address,
                         (dbg->cpu->eip == dbg->breakpoints[i].address) ? "<--" : "");
            }
        }
    }
}

// Draw status bar
static void draw_status(debugger_t *dbg, int y, int width) {
    const char *state_str;
    switch (dbg->cpu->state) {
        case CPU_RUNNING:  state_str = "RUNNING"; break;
        case CPU_HALTED:   state_str = "HALTED "; break;
        case CPU_EXCEPTION: state_str = "ERROR  "; break;
        default:           state_str = "UNKNOWN"; break;
    }
    
    attron(COLOR_PAIR(COLOR_MENU));
    mvprintw(y, 0, " CPU: %-8s | Steps: %-8u | EIP: 0x%08X | ESP: 0x%08X ",
             state_str, dbg->step_count, dbg->cpu->eip, dbg->cpu->esp);
    
    for (int i = 65; i < width; i++) {
        addch(' ');
    }
    attroff(COLOR_PAIR(COLOR_MENU));
}

// Draw help bar
static void draw_help(int y, int width) {
    attron(COLOR_PAIR(COLOR_MENU));
    mvprintw(y, 0, " [R=Run] [S=Step] [B=SetBP] [M=MemView] [X=Reset] [G=Goto] [Q=Quit] ");
    for (int i = 68; i < width; i++) {
        addch(' ');
    }
    attroff(COLOR_PAIR(COLOR_MENU));
}

// Simple input dialog for address
static uint32_t input_address(const char *prompt) {
    echo();
    curs_set(1);
    
    int height, width;
    getmaxyx(stdscr, height, width);
    
    // Draw input box
    int box_y = height / 2 - 2;
    int box_w = 50;
    int box_x = (width - box_w) / 2;
    
    draw_box(box_y, box_x, 5, box_w, prompt);
    mvprintw(box_y + 2, box_x + 2, "Address (hex): 0x");
    
    char input[16] = {0};
    mvgetnstr(box_y + 2, box_x + 18, input, 8);
    
    noecho();
    curs_set(0);
    
    return (uint32_t)strtoul(input, NULL, 16);
}

// Main debug loop
void debugger_run(debugger_t *dbg) {
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    // Initialize colors
    if (has_colors()) {
        start_color();
        init_pair(COLOR_REG_CHANGED, COLOR_YELLOW, COLOR_BLACK);
        init_pair(COLOR_FLAG_ON, COLOR_GREEN, COLOR_BLACK);
        init_pair(COLOR_FLAG_OFF, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_HIGHLIGHT, COLOR_BLACK, COLOR_CYAN);
        init_pair(COLOR_MENU, COLOR_BLACK, COLOR_WHITE);
        init_pair(COLOR_BORDER, COLOR_CYAN, COLOR_BLACK);
        init_pair(COLOR_TITLE, COLOR_CYAN, COLOR_BLACK);
    }
    
    dbg->mem_view_addr = 0;
    bool running = true;
    
    while (running) {
        clear();
        
        int height, width;
        getmaxyx(stdscr, height, width);
        
        // Layout: Left column (regs + flags + breakpoints), Right column (memory + disasm)
        int left_w = 32;
        int right_w = width - left_w - 1;
        
        // Draw panels
        draw_registers(dbg, 0, 0, left_w);
        draw_flags(dbg, 11, 0, left_w);
        draw_breakpoints(dbg, 16, 0, left_w);
        
        draw_memory(dbg, 0, left_w, right_w);
        draw_disassembly(dbg, 11, left_w, right_w, 10);
        
        // Draw bottom status bar
        draw_status(dbg, height - 2, width);
        draw_help(height - 1, width);
        
        refresh();
        
        // Handle input
        int ch = getch();
        
        switch (ch) {
            case 'r':
            case 'R':  // R: Run
                dbg->state = DBG_RUNNING;
                break;
                
            case 's':
            case 'S':  // S: Single step
                dbg->state = DBG_STEP;
                break;
                
            case 'b':
            case 'B':  // B: Set breakpoint at EIP
                debugger_add_breakpoint(dbg, dbg->cpu->eip);
                break;
                
            case 'm':
            case 'M':  // M: Memory view at EIP
                dbg->mem_view_addr = dbg->cpu->eip & 0xFFFFFFF0;
                break;
                
            case 'x':
            case 'X':  // X: Reset
                cpu_reset(dbg->cpu, 0, (uint32_t)dbg->memory->size);
                dbg->step_count = 0;
                dbg->state = DBG_PAUSED;
                memset(last_regs, 0, sizeof(last_regs));
                last_eflags = 0;
                break;
                
            case 'g':
            case 'G': {
                uint32_t addr = input_address("GOTO ADDRESS");
                if (addr != 0 || dbg->memory->size > 0) {
                    dbg->mem_view_addr = addr;
                }
                break;
            }
            
            case 'j':
            case 'J':
                dbg->mem_view_addr += 16;
                break;
                
            case 'k':
            case 'K':
                if (dbg->mem_view_addr >= 16) {
                    dbg->mem_view_addr -= 16;
                }
                break;
            
            case 'p':
            case 'P': {
                uint32_t addr = input_address("SET BREAKPOINT");
                if (addr != 0) {
                    debugger_add_breakpoint(dbg, addr);
                }
                break;
            }
                
            case 'q':
            case 'Q':
                running = false;
                break;
        }
        
        // Execute instruction
        if (dbg->state == DBG_RUNNING || dbg->state == DBG_STEP) {
            if (dbg->cpu->state == CPU_RUNNING) {
                cpu_step(dbg->cpu, dbg->memory);
                dbg->step_count++;
                
                // Check breakpoints
                if (debugger_check_breakpoint(dbg)) {
                    dbg->state = DBG_PAUSED;
                }
                
                // Single step mode: pause after one step
                if (dbg->state == DBG_STEP) {
                    dbg->state = DBG_PAUSED;
                }
                
                // Continuous run mode: keep executing until breakpoint/halt/key press
                if (dbg->state == DBG_RUNNING) {
                    nodelay(stdscr, TRUE);  // Non-blocking input
                    while (dbg->cpu->state == CPU_RUNNING && dbg->state == DBG_RUNNING) {
                        cpu_step(dbg->cpu, dbg->memory);
                        dbg->step_count++;
                        
                        // Check for key press to interrupt
                        int ch = getch();
                        if (ch != ERR) {
                            if (ch == 'q' || ch == 'Q') {
                                running = false;
                                break;
                            }
                            // Any other key pauses
                            dbg->state = DBG_PAUSED;
                            break;
                        }
                        
                        // Check breakpoints
                        if (debugger_check_breakpoint(dbg)) {
                            dbg->state = DBG_PAUSED;
                            break;
                        }
                        
                        // Periodically update display (every 1000 steps)
                        if (dbg->step_count % 1000 == 0) {
                            // Quick display update
                            draw_status(dbg, height - 2, width);
                            refresh();
                        }
                    }
                    nodelay(stdscr, FALSE);  // Restore blocking input
                    
                    // Check if halted
                    if (dbg->cpu->state != CPU_RUNNING) {
                        dbg->state = DBG_PAUSED;
                    }
                }
            }
        }
    }
    
    // Cleanup
    endwin();
}
