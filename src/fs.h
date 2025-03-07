#pragma once

#include <stddef.h>
#include <stdio.h>

typedef struct FileInfo {
    size_t size;
    char *data;
} file_info_t;

file_info_t *read_file(FILE *file);
