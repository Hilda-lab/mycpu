#ifndef MYCPU_ASSEMBLER_H
#define MYCPU_ASSEMBLER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Assembler result
typedef struct {
    uint8_t *code;          // Generated machine code
    size_t size;            // Code size in bytes
    char *error_msg;        // Error message (NULL if success)
    int error_line;         // Line number of error (0 if no error)
} asm_result_t;

// Assemble source code string
asm_result_t assemble(const char *source);

// Assemble from file
asm_result_t assemble_file(const char *filename);

// Free assembler result
void asm_result_free(asm_result_t *result);

// Write machine code to binary file
bool write_binary(const char *filename, const uint8_t *code, size_t size);

// Print disassembly of generated code
void print_disassembly(const uint8_t *code, size_t size);

#endif
