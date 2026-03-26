#ifndef MYCPU_LOADER_H
#define MYCPU_LOADER_H

#include "mycpu/memory.h"

bool loader_load_binary(memory_t *memory, const char *path, uint32_t load_addr, size_t *loaded_size);

#endif
