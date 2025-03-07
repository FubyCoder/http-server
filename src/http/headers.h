#pragma once
#include <stdlib.h>

typedef struct Header {
    char *name;
    char *value;
} header_t;

typedef struct HeaderList {
    size_t capacity;
    size_t length;
    header_t **data;
} header_list_t;

header_list_t *create_header_list(size_t initial_capacity);
int append_header_list(header_list_t *header_list, header_t *item);
void free_header_list(header_list_t *header_list);

void free_header(header_t *header);
header_t *create_header(char *name, char *value);
char *format_header_string(header_t *header);
