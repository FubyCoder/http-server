#include "status.h"
#include <stddef.h>

static const status_map_t status_map[] = {
    // Informational Responses (100–199)
    {.code = CONTINUE, .message = "Continue"},
    {.code = SWITCHING_PROTOCOLS, .message = "Switching Protocols"},
    {.code = PROCESSING, .message = "Processing"},
    {.code = EARLY_HINTS, .message = "Early Hints"},

    // Successful Responses (200–299)
    {.code = OK, .message = "OK"},
    {.code = CREATED, .message = "Created"},
    {.code = ACCEPTED, .message = "Accepted"},
    {.code = NON_AUTHORITATIVE_INFORMATION, .message = "Non-Authoritative Information"},
    {.code = NO_CONTENT, .message = "No Content"},

    // Redirection Responses (300–399)
    {.code = MULTIPLE_CHOICES, .message = "Multiple Choices"},
    {.code = MOVED_PERMANENTLY, .message = "Moved Permanently"},
    {.code = FOUND, .message = "Found"},
    {.code = NOT_MODIFIED, .message = "Not Modified"},

    // Client Error Responses (400–499)
    {.code = BAD_REQUEST, .message = "Bad Request"},
    {.code = UNAUTHORIZED, .message = "Unauthorized"},
    {.code = FORBIDDEN, .message = "Forbidden"},
    {.code = NOT_FOUND, .message = "Not Found"},

    // Server Error Responses (500–599)
    {.code = INTERNAL_SERVER_ERROR, .message = "Internal Server Error"},
    {.code = NOT_IMPLEMENTED, .message = "Not Implemented"},
    {.code = BAD_GATEWAY, .message = "Bad Gateway"},
    {.code = SERVICE_UNAVAILABLE, .message = "Service Unavailable"},
};

static const size_t STATUS_CODE_MAP_SIZE = sizeof(status_map) / sizeof(status_map[0]);

char *get_status_string(status_code_t code) {
    for (size_t i = 0; i < STATUS_CODE_MAP_SIZE; i++) {
        if (status_map[i].code == code) {
            return status_map[i].message;
        }
    }

    return "Unknown";
}
