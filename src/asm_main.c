#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mycpu/assembler.h"
#include "mycpu/cpu.h"
#include "mycpu/memory.h"
#include "mycpu/decoder.h"

void print_usage(const char *prog) {
    printf("myCPU Assembler\n\n");
    printf("Usage: %s [options] <input.asm>\n\n", prog);
    printf("Options:\n");
    printf("  -o <file>    Output binary file (default: a.out)\n");
    printf("  -r           Run the program after assembly\n");
    printf("  -d           Disassemble and show generated code\n");
    printf("  -h           Show this help\n\n");
    printf("Example:\n");
    printf("  %s -o program.bin program.asm\n", prog);
    printf("  %s -r program.asm\n", prog);
}

int main(int argc, char *argv[]) {
    const char *input_file = NULL;
    const char *output_file = "a.out";
    bool run_flag = false;
    bool disasm_flag = false;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        }
        else if (strcmp(argv[i], "-r") == 0) {
            run_flag = true;
        }
        else if (strcmp(argv[i], "-d") == 0) {
            disasm_flag = true;
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else if (argv[i][0] != '-') {
            input_file = argv[i];
        }
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    if (!input_file) {
        fprintf(stderr, "Error: No input file specified\n\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // Assemble
    printf("Assembling: %s\n", input_file);
    asm_result_t result = assemble_file(input_file);
    
    if (result.error_msg) {
        fprintf(stderr, "Error on line %d: %s\n", result.error_line, result.error_msg);
        asm_result_free(&result);
        return 1;
    }
    
    printf("Success! Generated %zu bytes\n", result.size);
    
    // Show disassembly
    if (disasm_flag) {
        printf("\nGenerated code:\n");
        printf("----------------\n");
        for (size_t i = 0; i < result.size; i++) {
            printf("%02X ", result.code[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        if (result.size % 16 != 0) printf("\n");
    }
    
    // Write output
    if (!write_binary(output_file, result.code, result.size)) {
        fprintf(stderr, "Error: Cannot write to %s\n", output_file);
        asm_result_free(&result);
        return 1;
    }
    printf("Output: %s\n", output_file);
    
    // Run if requested
    if (run_flag) {
        printf("\nRunning program...\n");
        printf("------------------\n");
        
        memory_t memory;
        cpu_t cpu;
        
        if (!memory_init(&memory, MYCPU_MEM_SIZE)) {
            fprintf(stderr, "Memory init failed\n");
            asm_result_free(&result);
            return 1;
        }
        
        // Load code
        for (size_t i = 0; i < result.size; i++) {
            memory_write8(&memory, (uint32_t)i, result.code[i]);
        }
        
        cpu_init(&cpu);
        cpu_reset(&cpu, 0, (uint32_t)memory.size);
        
        int steps = 0;
        while (cpu.state == CPU_RUNNING && steps < 10000) {
            printf("Step %d: EIP=0x%08X EAX=%u EBX=%u ECX=%u EDX=%u\n",
                   steps, cpu.eip, cpu.eax, cpu.ebx, cpu.ecx, cpu.edx);
            cpu_step(&cpu, &memory);
            steps++;
        }
        
        printf("\nFinal state:\n");
        printf("  CPU: %s\n", cpu.state == CPU_HALTED ? "HALTED" : 
                              cpu.state == CPU_EXCEPTION ? "ERROR" : "UNKNOWN");
        printf("  EAX = %u\n", cpu.eax);
        printf("  EBX = %u\n", cpu.ebx);
        printf("  ECX = %u\n", cpu.ecx);
        printf("  EDX = %u\n", cpu.edx);
        printf("  Steps: %d\n", steps);
        
        memory_free(&memory);
    }
    
    asm_result_free(&result);
    return 0;
}
