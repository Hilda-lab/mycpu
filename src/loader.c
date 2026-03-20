#include <stdio.h>

#include "mycpu/loader.h"

bool loader_load_binary(memory_t *memory, const char *path, uint32_t load_addr, size_t *loaded_size) {
    if (memory == NULL || memory->data == NULL || path == NULL || load_addr >= memory->size) {
        return false;
    }

    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return false;
    }

    size_t max_len = memory->size - load_addr;
    size_t nread = fread(&memory->data[load_addr], 1, max_len, fp);
    fclose(fp);

    if (loaded_size != NULL) {
        *loaded_size = nread;
    }
    return true;
}
