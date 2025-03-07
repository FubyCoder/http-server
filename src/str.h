#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct String {
    size_t capacity;
    size_t length;
    char *data;
} string_t;

string_t *create_string(size_t initial_capacity);

int append_rawchars(string_t *string, char *text, size_t text_length);
int append_string(string_t *string, char *text);
int append_char(string_t *string, char c);

char *int_to_str(long value);
int start_with(char *text, char *str);

char *get_extension(char *text);
