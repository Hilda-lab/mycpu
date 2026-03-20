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
