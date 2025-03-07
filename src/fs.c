#include <linux/limits.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "fs.h"

// This function reads all the content fro mthe file stream
// doesn't perform fclose at the end
file_info_t *read_file(FILE *file) {
    if (file == NULL) {
        return NULL;
    }

    char file_chunk[1024];
    long file_buffer_size = 0;
    long file_buffer_capacity = 1024;
    size_t read_count = -1;

    char *file_buffer = malloc(1024);

    if (file_buffer == NULL) {
        return NULL;
    }

    while ((read_count = fread(file_chunk, 1, 1, file)) != 0) {
        if (file_buffer_size + 1 > file_buffer_capacity) {
            size_t new_capacity = file_buffer_capacity * 2 + 1;
            char *new_file_buffer = realloc(file_buffer, new_capacity);

            if (new_file_buffer == NULL) {
                // TODO handle error
                exit(1);
            }

            file_buffer = new_file_buffer;
            file_buffer_capacity = new_capacity;
        }

        memmove(file_buffer + file_buffer_size, file_chunk, 1);
        file_buffer_size += 1;
    }

    // file_buffer[file_buffer_size] = '\0';

    file_info_t *info = malloc(sizeof(file_info_t));

    if (info == NULL) {
        free(file_buffer);
        return NULL;
    }

    info->data = file_buffer;
    info->size = file_buffer_size;

    return info;
}
