#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "str.h"

string_t *create_string(size_t initial_capacity) {
    string_t *string = malloc(sizeof(string_t));

    if (string == NULL) {
        return NULL;
    }

    size_t capacity = initial_capacity;
    size_t length = 0;
    char *data = calloc(sizeof(char), capacity);

    if (data == NULL) {
        free(string);
        return NULL;
    }

    string->capacity = capacity;
    string->length = length;
    string->data = data;

    return string;
}

int append_rawchars(string_t *string, char *text, size_t text_length) {
    if (string->length + text_length + 1 > string->capacity) {
        size_t new_capacity = string->capacity * 2 + text_length;
        char *new_data = realloc(string->data, new_capacity);

        if (new_data == NULL) {
            return -1;
        }

        string->data = new_data;
        string->capacity = new_capacity;
    }

    memmove(string->data + string->length, text, text_length);
    string->length += text_length;

    return 0;
}

/**
text must be a null terminated string
*/
int append_string(string_t *string, char *text) {
    size_t text_length = strlen(text);

    if (string->length + text_length + 1 > string->capacity) {
        size_t new_capacity = string->capacity * 2 + text_length;
        char *new_data = realloc(string->data, new_capacity);

        if (new_data == NULL) {
            return -1;
        }

        string->data = new_data;
        string->capacity = new_capacity;
    }

    strcat(string->data, text);
    string->length += text_length;

    return 0;
}

int append_char(string_t *string, char c) {
    if (string->length == string->capacity) {
        size_t new_capacity = string->capacity * 2 + 1;
        char *new_data = realloc(string->data, new_capacity);

        if (new_data == NULL) {
            return -1;
        }

        string->data = new_data;
        string->capacity = new_capacity;
    }
    string->data[string->length] = c;
    string->length += 1;
    string->data[string->length] = '\0';

    return 0;
}

/**
Returns a dynamically allocated string that represent the integer provided
*/
char *int_to_str(long value) {
    size_t length = 0;
    int is_negative = value < 0 ? 1 : 0;

    if (is_negative) {
        value *= -1;
    }

    long new_value = value;
    // Doing Log10 (not an exact log10 but its only used to get the length of the number as characters)
    do {
        new_value = new_value / 10;
        length++;
    } while (new_value);

    // +1 for \0 (Null terminator)
    char *str = malloc(length + 1 + is_negative);

    if (str == NULL) {
        return NULL;
    }

    new_value = value;
    long prev_value = value;
    int modulo = 0;
    size_t index = 0;

    if (is_negative) {
        str[index] = '-';
        index++;
    }

    do {
        new_value = new_value / 10;
        modulo = prev_value - new_value * 10;
        prev_value = new_value;
        str[(length - 1) - index + is_negative] = '0' + modulo;
        index++;
    } while (new_value);

    if (is_negative) {
        str[0] = '-';
    }

    str[index] = '\0';
    return str;
}

/**
 * if the text string starts with str returns 1
 * otherwise 0
 */
int start_with(char *text, char *str) {
    size_t text_len = strlen(text);
    size_t str_len = strlen(str);

    if (text_len < str_len) {
        return 0;
    }

    return memcmp(text, str, str_len) == 0 ? 1 : 0;
}

char *get_extension(char *text) {
    return strrchr(text, '.');
}
