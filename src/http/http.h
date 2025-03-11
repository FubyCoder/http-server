#pragma once
#include "headers.h"

typedef struct HttpVersion {
    long major;
    long minor;
} http_version_t;

typedef struct Request {
    char *method;
    char *uri;
    http_version_t *version;
    header_list_t *headers;
    char *body;
} request_t;

typedef struct Response {
    int status;
    header_list_t *headers;
    char *body;
    size_t body_length;
} response_t;
