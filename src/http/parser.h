#pragma once

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "headers.h"
#include "http.h"
#include "parser_helpers.h"

int expect_char(char **buffer, size_t *i, char expected);

char *extract_token(char **buffer, size_t buffer_size, size_t *index);
char *extract_method(char **buffer, size_t buffer_size, size_t *index);

char *extract_uri(char **buffer, size_t buffer_size, size_t *index);
long extract_int(char **buffer, size_t buffer_size, size_t *index);

char *extract_text(char **buffer, size_t buffer_size, size_t *index);

header_t *extract_header(char **buffer, size_t buffer_size, size_t *index);
http_version_t *extract_http_version(char **buffer, size_t buffer_size, size_t *index);
