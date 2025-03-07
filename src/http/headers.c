#include "headers.h"
#include <stdlib.h>
#include <string.h>

header_list_t *create_header_list(size_t initial_capacity) {
    header_list_t *list = malloc(sizeof(header_list_t));

    if (list == NULL) {
        return NULL;
    }

    header_t **data = calloc(initial_capacity, sizeof(header_t *));
    if (data == NULL) {
        free(list);
        return NULL;
    }

    list->data = data;
    list->capacity = initial_capacity;
    list->length = 0;

    return list;
}

/**
Returns
- -1 if append fails
- 0 if succeed
*/
int append_header_list(header_list_t *header_list, header_t *item) {
    if (header_list->length == header_list->capacity) {
        int new_capacity = header_list->capacity + 2;
        header_t **new_data = realloc(header_list->data, sizeof(header_t) * (new_capacity));

        if (new_data == NULL) {
            return -1;
        }

        header_list->data = new_data;
        header_list->capacity = new_capacity;
    }

    header_list->data[header_list->length++] = item;
    return 0;
}

void free_header(header_t *header) {
    if (header->name != NULL) {
        free(header->name);
    }

    if (header->value != NULL) {
        free(header->value);
    }

    if (header != NULL) {
        free(header);
    }
}

void free_header_list(header_list_t *header_list) {
    if (header_list == NULL) {
        return;
    }

    for (size_t i = 0; i < header_list->length; i++) {
        free_header(header_list->data[i]);
    }

    free(header_list->data);
    free(header_list);
}

header_t *create_header(char *name, char *value) {
    header_t *header = malloc(sizeof(header_t));
    if (header == NULL) {
        return NULL;
    }

    header->name = name;
    header->value = value;

    return header;
}

// Returns malloc allocated string
char *format_header_string(header_t *header) {
    if (header == NULL) {
        return NULL;
    }

    // Why +5?
    // - ':' charcater after the name
    // - ' ' space charcater after :
    // - '\r' after the value
    // - '\n' after the \r
    // - '\0' after the \n
    // Example "Content-Type: 40\r\n\0"
    size_t name_len = strlen(header->name);
    size_t value_len = strlen(header->value);
    char *text = malloc(name_len + value_len + 5);

    if (text == NULL) {
        return NULL;
    }

    // THIS IS TOTAL SHIT OF CODE
    // FIXME make this prettier
    size_t index = 0;
    memcpy(text, header->name, name_len);
    index += name_len;

    text[index++] = ':';
    text[index++] = ' ';

    memcpy(text + index, header->value, value_len);
    index += value_len;

    text[index++] = '\r';
    text[index++] = '\n';
    text[index++] = '\0';

    return text;
}
