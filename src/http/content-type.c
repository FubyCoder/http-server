#include "headers.h"
#include <string.h>

header_t *get_content_type_header(char *extension) {
    header_t *content_type;

    if (strcmp(extension, ".js") == 0) {
        content_type = create_header("Content-Type", "text/javascript");
    } else if (strcmp(extension, ".json") == 0) {
        content_type = create_header("Content-Type", "application/json");
    } else if (strcmp(extension, ".css") == 0) {
        content_type = create_header("Content-Type", "text/css");
    } else if (strcmp(extension, ".ttf") == 0) {
        content_type = create_header("Content-Type", "font/ttf");
    } else if (strcmp(extension, ".otf") == 0) {
        content_type = create_header("Content-Type", "font/otf");
    } else if (strcmp(extension, ".woff") == 0) {
        content_type = create_header("Content-Type", "font/woff");
    } else if (strcmp(extension, ".woff2") == 0) {
        content_type = create_header("Content-Type", "font/woff2");
    } else if (strcmp(extension, ".jpeg") == 0 || strcmp(extension, ".jpg") == 0) {
        content_type = create_header("Content-Type", "image/jpeg");
    } else if (strcmp(extension, ".png") == 0) {
        content_type = create_header("Content-Type", "image/png");
    } else if (strcmp(extension, ".svg") == 0) {
        content_type = create_header("Content-Type", "image/svg+xml");
    } else if (strcmp(extension, ".ico") == 0) {
        content_type = create_header("Content-Type", "image/vnd.microsoft.icon");
    } else if (strcmp(extension, ".html") == 0) {
        content_type = create_header("Content-Type", "text/html");
    } else {
        content_type = create_header("Content-Type", "text/plain");
    }

    return content_type;
}
