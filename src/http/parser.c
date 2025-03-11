#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "headers.h"
#include "http.h"
#include "parser_helpers.h"

// IF the character is matched we consume it by incementing i
// TODO: use buffer_size and avoid overflow
int expect_char(char **buffer, size_t *i, char expected) {
    if ((*buffer)[*i] == expected) {
        *i = *i + 1;
        return 0;
    }

    return -1;
}

char *extract_token(char **buffer, size_t buffer_size, size_t *index) {
    size_t i = *index;
    size_t l = i;
    char *buf = *buffer;

    while (is_token(buf[l])) {
        if (l > buffer_size) {
            return NULL;
        }
        l++;
    }

    const int size = l - i;
    if (size <= 0) {
        return NULL;
    }

    char *extracted = malloc(size + 1);
    if (extracted == NULL) {
        return NULL;
    }

    memcpy(extracted, buf + i, size);
    extracted[size] = '\0';

    *index = l;

    return extracted;
}

char *extract_method(char **buffer, size_t buffer_size, size_t *index) {
    size_t i = *index;
    size_t l = i;
    char *buf = *buffer;

    while (is_alpha(buf[l])) {
        if (l < buffer_size) {
            l++;
        } else {
            return NULL;
        }
    }

    const size_t method_size = l - i;
    if (method_size == 0) {
        return NULL;
    }

    char *method_extract = malloc(method_size + 1);

    if (method_extract == NULL) {
        // FAILED TO ALLOCATE MEMORY
        return NULL;
    }

    memcpy(method_extract, buf + i, method_size);
    method_extract[method_size] = '\0';
    *index = l;

    return method_extract;
}

char *extract_uri(char **buffer, size_t buffer_size, size_t *index) {
    size_t i = *index;
    size_t l = i;
    char *buf = *buffer;

    // TODO implement other spec of http with hex string and some special chars
    while (l + 1 < buffer_size &&

           (is_alpha(buf[l]) || buf[l] == '/' || buf[l] == '.' || buf[l] == '_' || buf[l] == '-' || is_numeric(buf[l]))

    ) {
        l++;
    }

    const int size = l - i;
    if (size <= 0) {
        return NULL;
    }

    char *extracted = malloc(size + 1);

    if (extracted == NULL) {
        return NULL;
    }

    memcpy(extracted, buf + i, size);
    extracted[size] = '\0';
    *index = l;

    return extracted;
}

// TODO: use buffer_size and avoid overflow
long extract_int(char **buffer, size_t buffer_size, size_t *index) {
    size_t i = *index;
    size_t l = i;
    char *buf = *buffer;

    while (l < buffer_size && isdigit(buf[l])) {
        l++;
    }

    const int size = l - i;
    if (size <= 0) {
        return -1;
    }

    char text[size + 1];

    memcpy(text, buf + i, size);
    text[size] = '\0';

    long number = strtol(text, NULL, 10);
    *index = l;

    return number;
}

// TODO implement LWS checks as HTTP/1.0 spec
char *extract_text(char **buffer, size_t buffer_size, size_t *index) {
    size_t i = *index;
    size_t l = i;
    char *buf = *buffer;

    while (l < buffer_size && !is_ctl(buf[l])) {
        l++;
    }

    const int size = l - i;
    if (size <= 0) {
        return NULL;
    }

    char *extracted = malloc(size + 1);
    if (extracted == NULL) {
        return NULL;
    }

    memcpy(extracted, buf + i, size);
    extracted[size] = '\0';
    *index = l;

    return extracted;
}

header_t *extract_header(char **buffer, size_t buffer_size, size_t *index) {
    char *name = extract_token(buffer, buffer_size, index);
    if (name == NULL) {
        return NULL;
    }

    if (expect_char(buffer, index, ':') != 0) {
        free(name);
        return NULL;
    }

    if (expect_char(buffer, index, ' ') != 0) {
        free(name);
        return NULL;
    }

    char *value = extract_text(buffer, buffer_size, index);
    if (value == NULL) {
        free(name);
        return NULL;
    }

    if (expect_char(buffer, index, '\r') != 0) {
        free(name);
        free(value);
        return NULL;
    }

    if (expect_char(buffer, index, '\n') != 0) {
        free(name);
        free(value);
        return NULL;
    }

    header_t *header = malloc(sizeof(header_t));
    if (header == NULL) {
        free(name);
        free(value);
        return NULL;
    }

    header->name = name;
    header->value = value;

    return header;
}

/**
 * As per HTTP/1.0 SPEC
 * The HTTP Version has the following format:
 *
 * HTTP/<INT>.<INT>
 *
 * Where the first INT indicates the major version
 * and the second INT indiicates the minor version
 *
 * so 1.1 > 1.01 and 1.12 > 1.1
 * and 2.1 > 1.11
 */
http_version_t *extract_http_version(char **buffer, size_t buffer_size, size_t *index) {
    if (expect_char(buffer, index, 'H') != 0) {
        return NULL;
    }

    if (expect_char(buffer, index, 'T') != 0) {
        return NULL;
    }

    if (expect_char(buffer, index, 'T') != 0) {
        return NULL;
    }

    if (expect_char(buffer, index, 'P') != 0) {
        return NULL;
    }

    if (expect_char(buffer, index, '/') != 0) {
        return NULL;
    }

    long major = extract_int(buffer, buffer_size, index);

    if (major == -1) {
        return NULL;
    }

    if (expect_char(buffer, index, '.') != 0) {
        return NULL;
    }

    long minor = extract_int(buffer, buffer_size, index);
    if (minor == -1) {
        return NULL;
    }

    http_version_t *version = malloc(sizeof(http_version_t));
    if (version == NULL) {
        return NULL;
    }

    version->major = major;
    version->minor = minor;

    return version;
}
