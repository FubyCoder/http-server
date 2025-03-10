#include "headers.h"
#include <string.h>

header_t *get_content_type_header(char *extension) {
    if (strcmp(extension, ".js") == 0) {
        return create_header("Content-Type", "text/javascript");
    }

    if (strcmp(extension, ".json") == 0) {
        return create_header("Content-Type", "application/json");
    }

    if (strcmp(extension, ".css") == 0) {
        return create_header("Content-Type", "text/css");
    }

    if (strcmp(extension, ".ttf") == 0) {
        return create_header("Content-Type", "font/ttf");
    }

    if (strcmp(extension, ".otf") == 0) {
        return create_header("Content-Type", "font/otf");
    }

    if (strcmp(extension, ".woff") == 0) {
        return create_header("Content-Type", "font/woff");
    }

    if (strcmp(extension, ".woff2") == 0) {
        return create_header("Content-Type", "font/woff2");
    }

    if (strcmp(extension, ".jpeg") == 0 || strcmp(extension, ".jpg") == 0) {
        return create_header("Content-Type", "image/jpeg");
    }

    if (strcmp(extension, ".png") == 0) {
        return create_header("Content-Type", "image/png");
    }

    if (strcmp(extension, ".svg") == 0) {
        return create_header("Content-Type", "image/svg+xml");
    }

    if (strcmp(extension, ".ico") == 0) {
        return create_header("Content-Type", "image/vnd.microsoft.icon");
    }

    if (strcmp(extension, ".html") == 0) {
        return create_header("Content-Type", "text/html");
    }

    return create_header("Content-Type", "text/plain");
}
