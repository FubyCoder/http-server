#include <stdlib.h>

#include "fs.h"

// This function reads all the content fro mthe file stream
// doesn't perform fclose at the end
file_info_t *read_file(FILE *file) {
    if (file == NULL) {
        return NULL;
    }

    const int CHUNK_SIZE = 1024;
    size_t size = 0;
    size_t capacity = CHUNK_SIZE;
    size_t read_size = -1;

    char *buffer = malloc(CHUNK_SIZE);

    size_t new_capacity;
    char *tmp_buffer;

    if (buffer == NULL) {
        return NULL;
    }

    while ((read_size = fread(buffer + size, 1, CHUNK_SIZE, file)) != 0) {
        if (size + CHUNK_SIZE >= capacity) {
            new_capacity = capacity * 2;
            tmp_buffer = realloc(buffer, new_capacity);

            if (tmp_buffer == NULL) {
                free(buffer);
                return NULL;
            }

            buffer = tmp_buffer;
            capacity = new_capacity;
        }

        size += read_size;
    }

    // Realloc to remove unused memory
    tmp_buffer = realloc(buffer, size + 1);

    if (tmp_buffer == NULL) {
        free(buffer);
        return NULL;
    }

    buffer = tmp_buffer;
    buffer[size] = '\0';

    file_info_t *info = malloc(sizeof(file_info_t));

    if (info == NULL) {
        free(buffer);
        return NULL;
    }

    info->data = buffer;
    info->size = size;

    return info;
}
