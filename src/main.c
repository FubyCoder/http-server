#include <ctype.h>
#include <errno.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

const char *base_html_page = "<!DOCTYPE html><html><body><h1>HEY</h1></body></html>\0";

typedef struct HTTP_VERSION {
    long major;
    long minor;
} http_version_t;

typedef struct Header {
    char *name;
    char *value;
} header_t;

typedef struct HeaderList {
    size_t capacity;
    size_t length;
    header_t **data;
} header_list_t;

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

void print_header(header_t *header) {
    printf("header : %s : %s\n", header->name, header->value);
}

void print_header_list(header_list_t *list) {
    for (size_t i = 0; i < list->length; i++) {
        print_header(list->data[i]);
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

typedef struct Request {
    char *method;
    char *uri;
    char *version;

    header_list_t *headers;

    struct Query {
        char *name;
        char *value;
    } *query;

    char *body;
} request_t;

/**
Returns 1 when true
for alpha is any character between A-Z and a-z
*/
int is_alpha(char c) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        return 1;
    }

    return 0;
}

int is_numeric(char c) {
    if (c >= '0' && c <= '9') {
        return 1;
    }

    return 0;
}

int is_ctl(char c) {
    // Values between 0 - 31 in base 8 (octal) and value number 127 (0177 in base
    // 8) specified in HTTP/1.0 spec sec 2.2
    if ((c >= 00 && c <= 031) || c == 0177) {
        return 1;
    }

    return 0;
}

// tspecials from HTTP/1.0 spec
int is_tspecial(char c) {
    switch (c) {
    case ('('):
    case (')'):
    case ('<'):
    case ('>'):
    case ('@'):
    case (','):
    case (';'):
    case (':'):
    case ('\\'):
    case ('"'):
    case ('/'):
    case ('['):
    case (']'):
    case ('?'):
    case ('='):
    case ('{'):
    case ('}'):
    case (' '):
    case (0x9):
        return 1;
    default:
        return 0;
    }
}

int is_token(char c) {
    if (!is_ctl(c) && !is_tspecial(c)) {
        return 1;
    }

    return 0;
}

int is_hex(char c) {
    if (is_numeric(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
        return 1;
    }

    return 0;
}

// IF the character is matched we consume it by incementing i
// TODO: use buffer_size and avoid overflow
int expect_char(char **buffer, int *i, char expected) {
    if ((*buffer)[*i] == expected) {
        *i = *i + 1;
        return 0;
    }

    return -1;
}

char *extract_token(char **buffer, int buffer_size, int *index) {
    int i = *index;
    int l = i;
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

char *extract_method(char **buffer, int buffer_size, int *index) {
    int i = *index;
    int l = i;
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

char *extract_uri(char **buffer, int buffer_size, int *index) {
    int i = *index;
    int l = i;
    char *buf = *buffer;

    // TODO implement other spec of http with hex string and some special chars
    while (is_alpha(buf[l]) || buf[l] == '/' || buf[l] == '.' || is_numeric(buf[l])) {
        if (l >= buffer_size) {
            printf("L OUT OF BUFFER_SIZE\n");
            return NULL;
        }

        l++;
    }

    const int size = l - i;
    if (size <= 0) {
        printf("SIZE is NEGATIVE : %i\n", size);
        printf("I: %i, L: %i\n", i, l);
        return NULL;
    }

    char *extracted = malloc(size + 1);

    if (extracted == NULL) {
        printf("MALLOC FAIL\n");
        // FAILED TO ALLOCATE MEMORY
        return NULL;
    }

    memcpy(extracted, buf + i, size);
    extracted[size] = '\0';
    *index = l;

    return extracted;
}

// TODO: use buffer_size and avoid overflow
long extract_int(char **buffer, int buffer_size, int *index) {
    int i = *index;
    int l = i;
    char *buf = *buffer;

    while (isdigit(buf[l])) {
        if (l > buffer_size) {
            return -1;
        }
        l++;
    }

    const int size = l - i;
    if (size <= 0) {
        return -1;
    }

    char *extracted = malloc(size + 1);
    if (extracted == NULL) {
        return -1;
    }

    memcpy(extracted, buf + i, size);
    extracted[size] = '\0';

    long number = strtol(extracted, NULL, 10);
    *index = l;

    free(extracted);

    return number;
}

// TODO implement LWS checks as HTTP/1.0 spec
char *extract_text(char **buffer, int buffer_size, int *index) {
    int i = *index;
    int l = i;
    char *buf = *buffer;

    while (!is_ctl(buf[l])) {
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

header_t *extract_header(char **buffer, int buffer_size, int *index) {
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
http_version_t *extract_http_version(char **buffer, int buffer_size, int *index) {
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

void parse_request(int client_fd) {
    const int CHUNK_SIZE = 512;
    char *buffer = calloc(CHUNK_SIZE, 1);

    if (buffer == NULL) {
        return;
    }

    size_t buffer_size = 0;
    size_t buffer_capacity = CHUNK_SIZE;

    char chunk[CHUNK_SIZE];
    long read_result;

    while (1) {
        read_result = read(client_fd, chunk, CHUNK_SIZE);

        if (read_result == -1) {
            // TODO handle error
            return;
        }

        if (buffer_size + read_result >= buffer_capacity) {
            char *new_buffer = realloc(buffer, buffer_capacity + CHUNK_SIZE);
            if (new_buffer == NULL) {
                return;
            }
            buffer = new_buffer;
            buffer_capacity = buffer_capacity + CHUNK_SIZE;
        }

        memcpy(buffer + buffer_size, chunk, read_result);
        buffer_size += read_result;

        if (read_result == 0 || read_result < CHUNK_SIZE) {
            // ADD null terminator
            if (buffer_size == buffer_capacity) {

                char *new_buffer = realloc(buffer, buffer_capacity + 1);
                if (new_buffer == NULL) {
                    return;
                }

                buffer = new_buffer;
                buffer_capacity++;
            }

            buffer[buffer_size] = '\0';
            buffer_size++;
            break;
        }
    }

    // 0 is a simple request - 1 is not
    int simple_request_candidate = 1;
    char *method = NULL;
    char *uri = NULL;
    http_version_t *version = NULL;
    header_list_t *header_list = NULL;
    char *body = NULL;

    // Body Content Length must be extracted from the headers
    // However we should also check that the data is there
    long content_length = 0;

    int line = 0;

    int i = 0;

    method = extract_method(&buffer, buffer_size, &i);
    if (method == NULL) {
        free(buffer);
        return;
    }

    simple_request_candidate = strcmp(method, "GET");

    // AS for HTML 1 spec we have a space after a METHOD
    if (expect_char(&buffer, &i, ' ') == -1) {
        free(buffer);
        free(method);
        return;
    }

    uri = extract_uri(&buffer, buffer_size, &i);
    if (uri == NULL) {
        free(buffer);
        free(method);
        return;
    }

    // AS for HTML 1 spec we have a space after a URI
    if (expect_char(&buffer, &i, ' ') == -1) {
        free(buffer);
        free(method);
        free(uri);
        return;
    }

    version = extract_http_version(&buffer, buffer_size, &i);
    if (version == NULL) {
        if (simple_request_candidate == 0) {
            // IF NO VERSION we are receiving an HTTP/0.9 request so we can also
            // fallback
            version = malloc(sizeof(http_version_t));
            if (version == NULL) {
                free(buffer);
                free(method);
                free(uri);
                return;
            }

            version->major = 0;
            version->minor = 9;

        } else {
            free(buffer);
            free(method);
            free(uri);
            return;
        }
    } else {
        simple_request_candidate = 1;
    }

    if (expect_char(&buffer, &i, '\r') == -1) {
        free(buffer);
        free(method);
        free(uri);
        return;
    }

    if (expect_char(&buffer, &i, '\n') == -1) {
        free(buffer);
        free(method);
        free(uri);

        return;
    }

    // No longer in HTTP/0.9 land
    if (simple_request_candidate != 0) {

        header_list = create_header_list(5);
        if (header_list == NULL) {
            free(buffer);
            free(method);
            free(uri);
            return;
        }

        // Why this strange check ?
        // IN HTTP spec we have atleast 4 charcater that separates the headers to
        // the body As we see in the spec we have STATUS_LINE *(HEADERS) CRLF [BODY]
        // Each section ends with CRLF so even if we dont have any header we can
        // still consume the 4 tokens
        while (i + 1 < buffer_size &&
               !(buffer[i - 2] == '\r' && buffer[i - 1] == '\n' && buffer[i] == '\r' && buffer[i + 1] == '\n')) {

            header_t *header = extract_header(&buffer, buffer_size, &i);
            if (header == NULL) {
                free(buffer);
                free(method);
                free(uri);
                free_header_list(header_list);
                return;
            }

            // TODO header names are case insensitive as HTTP/1.0 spec
            // We should lowercase them before comparing
            printf("headerName : '%s'\n", header->name);
            if (strcmp("Content-Length\0", header->name) == 0) {
                content_length = strtol(header->value, NULL, 10);
            }

            append_header_list(header_list, header);
        }
        // Increment to skip the \r and \n checks since they are done in the while
        // loop
        // TODO avoid overflow of buffer:
        i += 2;
    }

    if (content_length > 0) {
        body = malloc(content_length + 1);

        if (body == NULL) {
            free(method);
            free(uri);
            free(version);
            free(buffer);
            free_header_list(header_list);
            return;
        }

        if (i + content_length > buffer_size) {
            // TODO handle error
            // INVALID CONTENT_LENGHT provided
            free(method);
            free(uri);
            free(version);
            free(buffer);
            free_header_list(header_list);
            return;
        }

        memcpy(body, buffer + i, content_length);
        body[content_length] = '\0';
    }

    printf("METHOD: %s\n", method);
    printf("URI: %s\n", uri);
    printf("VERSION: %li.%li\n", version->major, version->minor);
    printf("Candidate simple: %i\n", simple_request_candidate);
    printf("Content-Length: %li\n", content_length);

    if (header_list != NULL) {
        print_header_list(header_list);
    }

    if (body != NULL) {
        printf("BODY: %s\n", body);
    }

    // TODO remove this when the function returns a request_t struct
    free(method);
    free(uri);
    free(version);
    free(buffer);
    free_header_list(header_list);
}

int main(int argc, char **argv) {
    struct sockaddr_in address;
    int fd;

    int port = 3000;

    if (argc > 1) {
        port = atoi(argv[1]);
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd == -1) {
        printf("Failed to open a socket\n");
        return -1;
    }

    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_family = AF_INET;

    if (bind(fd, (struct sockaddr_in *)&address, sizeof(struct sockaddr_in)) == -1) {

        if (errno == EADDRINUSE) {
            printf("Port %i already in use\n", port);
            return -1;
        } else {
            printf("Failed to bind socket\n");
        }

        return -1;
    }

    printf("Socket ready at port %i\n", port);

    if (listen(fd, 10) == -1) {
        printf("Error \n");
        return -1;
    }

    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_len;
        int client_fd;

        client_fd = accept(fd, &client_address, &client_len);
        if (client_fd == -1) {
            printf("Failed to connect to client\n");
            continue;
        }

        parse_request(client_fd);

        // SIMPLE REQUEST = GET <uri> <CRLF>

        // "HTTP/1.0 200 OK\n"
        //                  "\r\n"
        //
        char *response = "HTTP/1.0 200 OK\r\n"
                         "Content-Type: text/html\r\n"
                         "Content-Lenght: 40\r\n\r\n"
                         "<html><body><h1>HEY</h1></body></html>\0";

        send(client_fd, response, strlen(response), 0);

        if (shutdown(client_fd, SHUT_RDWR) == -1) {
            printf("Failed to close client connection\n");
            continue;
        }

        close(client_fd);
    }
    return 0;
}
