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
    if (memory == NULL || memory->data == NULL || addr >= memory->size || value == NULL) {
        return false;
    }
    *value = memory->data[addr];
    return true;
}

bool memory_read16(const memory_t *memory, uint32_t addr, uint16_t *value) {
    uint8_t lo = 0;
    uint8_t hi = 0;
    if (value == NULL) {
        return false;
    }
    if (!memory_read8(memory, addr, &lo) || !memory_read8(memory, addr + 1u, &hi)) {
        return false;
    }
    *value = (uint16_t)(lo | ((uint16_t)hi << 8u));
    return true;
}

bool memory_read32(const memory_t *memory, uint32_t addr, uint32_t *value) {
    uint8_t b0 = 0;
    uint8_t b1 = 0;
    uint8_t b2 = 0;
    uint8_t b3 = 0;
    if (value == NULL) {
        return false;
    }
    if (!memory_read8(memory, addr, &b0) || !memory_read8(memory, addr + 1u, &b1) ||
        !memory_read8(memory, addr + 2u, &b2) || !memory_read8(memory, addr + 3u, &b3)) {
        return false;
    }
    *value = ((uint32_t)b0) | ((uint32_t)b1 << 8u) | ((uint32_t)b2 << 16u) | ((uint32_t)b3 << 24u);
    return true;
}

bool memory_write8(memory_t *memory, uint32_t addr, uint8_t value) {
    if (memory == NULL || memory->data == NULL || addr >= memory->size) {
        return false;
    }
    memory->data[addr] = value;
    return true;
}

bool memory_write16(memory_t *memory, uint32_t addr, uint16_t value) {
    if (!memory_write8(memory, addr, (uint8_t)(value & 0xFFu)) ||
        !memory_write8(memory, addr + 1u, (uint8_t)((value >> 8u) & 0xFFu))) {
        return false;
    }
    return true;
}

bool memory_write32(memory_t *memory, uint32_t addr, uint32_t value) {
    if (!memory_write8(memory, addr, (uint8_t)(value & 0xFFu)) ||
        !memory_write8(memory, addr + 1u, (uint8_t)((value >> 8u) & 0xFFu)) ||
        !memory_write8(memory, addr + 2u, (uint8_t)((value >> 16u) & 0xFFu)) ||
        !memory_write8(memory, addr + 3u, (uint8_t)((value >> 24u) & 0xFFu))) {
        return false;
    }
    return true;
}
