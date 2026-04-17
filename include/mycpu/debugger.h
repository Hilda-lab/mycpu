#ifndef MYCPU_DEBUGGER_H
#define MYCPU_DEBUGGER_H

#include "mycpu/cpu.h"
#include "mycpu/memory.h"

// Debugger state
typedef enum {
    DBG_RUNNING,    // Running
    DBG_PAUSED,     // Paused
    DBG_STEP,       // Single step
    DBG_EXIT        // Exit
} debugger_state_t;

// Breakpoint structure
typedef struct {
    uint32_t address;
    bool enabled;
} breakpoint_t;

#define MAX_BREAKPOINTS 32

// Debugger structure
typedef struct {
    cpu_t *cpu;
    memory_t *memory;
    debugger_state_t state;
    breakpoint_t breakpoints[MAX_BREAKPOINTS];
    int breakpoint_count;
    uint32_t step_count;
    uint32_t mem_view_addr;     // Memory view base address
    int selected_menu;          // Currently selected menu item
} debugger_t;

// Initialize debugger
void debugger_init(debugger_t *dbg, cpu_t *cpu, memory_t *memory);

// Add breakpoint
bool debugger_add_breakpoint(debugger_t *dbg, uint32_t address);

// Remove breakpoint
bool debugger_remove_breakpoint(debugger_t *dbg, uint32_t address);

// Check if breakpoint hit
bool debugger_check_breakpoint(debugger_t *dbg);

// Start visual debugger
void debugger_run(debugger_t *dbg);

#endif
