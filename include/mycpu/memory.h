#ifndef MYCPU_MEMORY_H
#define MYCPU_MEMORY_H

#include "mycpu/types.h"

#define MYCPU_MEM_SIZE (16u * 1024u * 1024u)

typedef struct {
    uint8_t *data;
    size_t size;
} memory_t;

bool memory_init(memory_t *memory, size_t size);
void memory_free(memory_t *memory);
bool memory_read8(const memory_t *memory, uint32_t addr, uint8_t *value);
bool memory_write8(memory_t *memory, uint32_t addr, uint8_t value);

#endif
