#include <stdlib.h>

#include "mycpu/memory.h"

bool memory_init(memory_t *memory, size_t size) {
    memory->data = (uint8_t *)calloc(size, sizeof(uint8_t));
    if (memory->data == NULL) {
        memory->size = 0;
        return false;
    }
    memory->size = size;
    return true;
}

void memory_free(memory_t *memory) {
    free(memory->data);
    memory->data = NULL;
    memory->size = 0;
}

bool memory_read8(const memory_t *memory, uint32_t addr, uint8_t *value) {
    if (addr >= memory->size || value == NULL) {
        return false;
    }
    *value = memory->data[addr];
    return true;
}

bool memory_write8(memory_t *memory, uint32_t addr, uint8_t value) {
    if (addr >= memory->size) {
        return false;
    }
    memory->data[addr] = value;
    return true;
}
