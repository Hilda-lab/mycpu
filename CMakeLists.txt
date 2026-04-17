cmake_minimum_required(VERSION 3.16)
project(mycpu C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(include)

add_library(mycpu_core
    src/cpu.c
    src/memory.c
    src/decoder.c
    src/exec.c
    src/loader.c
    src/uart.c
    src/assembler.c
)

add_executable(mycpu
    src/main.c
)
target_link_libraries(mycpu PRIVATE mycpu_core)

enable_testing()
add_executable(mycpu_tests
    tests/test_vm.c
)
target_link_libraries(mycpu_tests PRIVATE mycpu_core)
add_test(NAME mycpu_tests COMMAND mycpu_tests)

add_executable(test_detailed
    tests/test_detailed.c
)
target_link_libraries(test_detailed PRIVATE mycpu_core)

add_executable(test_simple
    tests/test_simple.c
)
target_link_libraries(test_simple PRIVATE mycpu_core)

add_executable(test_fib
    tests/test_fib.c
)
target_link_libraries(test_fib PRIVATE mycpu_core)

add_executable(test_new_inst
    tests/test_new_inst.c
)
target_link_libraries(test_new_inst PRIVATE mycpu_core)

# Visual debugger (ncurses)
add_executable(mycpu_debugger
    src/debugger_main.c
    src/debugger.c
)
target_link_libraries(mycpu_debugger PRIVATE mycpu_core ncurses)

# Assembler
add_executable(mycpu_asm
    src/asm_main.c
)
target_link_libraries(mycpu_asm PRIVATE mycpu_core)
