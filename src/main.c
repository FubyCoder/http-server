#include <ctype.h>
#include <errno.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "fs.h"
#include "http/content-type.h"
#include "http/headers.h"
#include "http/parser_helpers.h"
#include "http/status.h"
#include "http_thread.h"
#include "str.h"

const int MAX_THREAD_COUNT = 8;

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

request_t *parse_request(int client_fd) {
    const int CHUNK_SIZE = 512;
    char *buffer = calloc(CHUNK_SIZE, 1);

    if (buffer == NULL) {
        return NULL;
    }

    size_t buffer_size = 0;
    size_t buffer_capacity = CHUNK_SIZE;

    char chunk[CHUNK_SIZE];
    long read_result;

    while (1) {
        read_result = read(client_fd, chunk, CHUNK_SIZE);

        if (read_result == -1) {
            // TODO handle error
            return NULL;
        }

        if (buffer_size + read_result >= buffer_capacity) {
            char *new_buffer = realloc(buffer, buffer_capacity + CHUNK_SIZE);
            if (new_buffer == NULL) {
                return NULL;
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
                    return NULL;
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

    size_t i = 0;

    method = extract_method(&buffer, buffer_size, &i);
    if (method == NULL) {
        free(buffer);
        // FAILED TO EXTRACT METHOD
        return NULL;
    }

    simple_request_candidate = strcmp(method, "GET");

    // AS for HTML 1 spec we have a space after a METHOD
    if (expect_char(&buffer, &i, ' ') == -1) {
        free(buffer);
        free(method);
        return NULL;
    }

    uri = extract_uri(&buffer, buffer_size, &i);
    if (uri == NULL) {
        free(buffer);
        free(method);
        return NULL;
    }

    // AS for HTML 1 spec we have a space after a URI
    if (expect_char(&buffer, &i, ' ') == -1) {
        free(buffer);
        free(method);
        free(uri);
        return NULL;
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
                return NULL;
            }

            version->major = 0;
            version->minor = 9;

        } else {
            free(buffer);
            free(method);
            free(uri);
            return NULL;
        }
    } else {
        simple_request_candidate = 1;
    }

    if (expect_char(&buffer, &i, '\r') == -1) {
        free(buffer);
        free(method);
        free(uri);
        return NULL;
    }

    if (expect_char(&buffer, &i, '\n') == -1) {
        free(buffer);
        free(method);
        free(uri);

        return NULL;
    }

    // No longer in HTTP/0.9 land
    if (simple_request_candidate != 0) {
        header_list = create_header_list(5);

        if (header_list == NULL) {
            free(buffer);
            free(method);
            free(uri);
            return NULL;
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
                // Failed to extract an header
                return NULL;
            }

            // TODO header names are case insensitive as HTTP/1.0 spec
            // We should lowercase them before comparing
            if (strcmp("Content-Length", header->name) == 0) {
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
            return NULL;
        }

        if (i + content_length > buffer_size) {
            // TODO handle error
            // INVALID CONTENT_LENGHT provided
            free(method);
            free(uri);
            free(version);
            free(buffer);
            free_header_list(header_list);
            return NULL;
        }

        memcpy(body, buffer + i, content_length);
        body[content_length] = '\0';
    }

    request_t *request = malloc(sizeof(request_t));
    if (request == NULL) {
        free(method);
        free(uri);
        free(version);
        free(buffer);
        free(body);
        free_header_list(header_list);
        return NULL;
    }

    free(buffer);

    request->method = method;
    request->uri = uri;
    request->version = version;
    request->body = body;
    request->headers = header_list;

    return request;
}

string_t *create_response(request_t *request, response_t *response) {
    string_t *res = create_string(10);

    char *status_code = int_to_str(response->status);
    char *status_message = get_status_string(response->status);

    if (status_code == NULL) {
        // TODO make free_string_t function
        free(res->data);
        free(res);
        return NULL;
    }

    char *body_length = int_to_str(strlen(response->body));

    if (body_length == NULL) {
        // TODO make free_string_t function
        free(res->data);
        free(res);
        free(status_code);
        return NULL;
    }

    append_string(res, "HTTP/1.0");
    append_string(res, " ");
    append_string(res, status_code);
    append_string(res, " ");
    append_string(res, status_message);
    append_string(res, "\r\n");

    if (response->headers && response->headers->length > 0) {
        for (size_t i = 0; i < response->headers->length; i++) {
            char *header_str = format_header_string(response->headers->data[i]);
            append_string(res, header_str);
            free(header_str);
        }
    }

    append_string(res, "\r\n");

    append_rawchars(res, response->body, response->body_length);

    free(body_length);
    free(status_code);

    return res;
}

void handle_http_request(int client_fd, char *public_path) {
    request_t *request = parse_request(client_fd);
    if (request == NULL) {
        // TODO implement request reply for errors
        // We are either in 2 cases
        // - The request has failed to parse for some reason (like malformed)
        // - The request has failed to parse for memory issues (failed to malloc / realloc)
        // We should reply with a 500 or close the socket directly based on the situation
        // 500 -> An error by our end
        // socket close -> Malformed request
        return;
    }

    printf("[%s] %s\n", request->method, request->uri);

    header_list_t *header_list = create_header_list(3);
    response_t response = {
        .status = 200,
        .headers = header_list,
        .body = "<!DOCTYPE html><html><body><h1>NON ROOT :(</h1></body></html>",
    };

    if (strcmp(request->method, "GET") == 0) {
        // TODO make this section separate to handle filesystem

        char file_path[PATH_MAX] = "\0";
        strcat(file_path, public_path);
        // TODO verify uri length
        strcat(file_path, request->uri);

        long uri_len = strlen(request->uri);
        if (request->uri[uri_len - 1] == '/') {
            // IF ends with slash append index.html
            strcat(file_path, "index.html");
        }

        // Full path should be used only if resolved is equal to NULL
        // TODO usaged of full_path
        char *full_path = NULL;
        char *resolved = realpath(file_path, full_path);

        if (resolved == NULL) {
            // Failed to resolve path
            response.body = "<!DOCTYPE html><html><body><h1>File not found :(</h1></body></html>";
            response.body_length = strlen(response.body);
            response.status = 404;
        } else {
            // Check if path traversal is occurred
            if (!start_with(resolved, public_path)) {
                response.body = "<!DOCTYPE html><html><body><h1>File not found :(</h1></body></html>";
                response.body_length = strlen(response.body);
                response.status = 404;
            } else {
                char *extension = get_extension(resolved);

                // TODO handle NULL for content_type
                header_t *content_type = get_content_type_header(extension);
                append_header_list(response.headers, content_type);

                FILE *file = fopen(resolved, "rb");
                file_info_t *file_response = read_file(file);
                fclose(file);

                response.body = file_response->data;
                response.body_length = file_response->size;

                char *buff_size_len = int_to_str(file_response->size);
                header_t *content_length = create_header("Content-Length", buff_size_len);
                append_header_list(response.headers, content_length);

                free(full_path);
                free(file_response);
            }
        }
    }

    string_t *res = create_response(NULL, &response);

    send(client_fd, res->data, res->length, 0);

    // TODO replace this with free_string_t when made
    free(res->data);
    free(res);

    // TODO add free response.body when all bodies are heap allocated
    // TODO add free response.headers when all headers are heap allocated

    if (shutdown(client_fd, SHUT_RDWR) == -1) {
        printf("Failed to close client connection\n");
        return;
    }

    close(client_fd);
}

int main(int argc, char **argv) {
    http_task_t *tasks = malloc(sizeof(http_task_t) * 100);
    size_t tasks_count = 0;

    pthread_t threads[MAX_THREAD_COUNT];
    struct sockaddr_in address;
    int fd;

    int port = 3000;

    if (argc > 1) {
        port = atoi(argv[1]);
    }

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working dir: %s\n", cwd);
    }

    // TODO this should be more dynamic with a router struct
    // the struct should allow us to bind multiple directories
    // At the moment we only have one
    char public_path[PATH_MAX] = "\0";
    strcat(public_path, cwd);
    strcat(public_path, "/public");

    fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd == -1) {
        printf("Failed to open a socket\n");
        return EXIT_FAILURE;
    }

    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_family = AF_INET;

    if (bind(fd, (struct sockaddr_in *)&address, sizeof(struct sockaddr_in)) == -1) {

        if (errno == EADDRINUSE) {
            printf("Port %i already in use\n", port);
            return EXIT_FAILURE;
        } else {
            printf("Failed to bind socket\n");
        }

        return EXIT_FAILURE;
    }

    if (listen(fd, 10) == -1) {
        return EXIT_FAILURE;
    }

    printf("Socket ready at port %i\n", port);

    // INITIALIZE THREADS FOR THREAD POOL

    setup_http_tasks();
    for (int i = 0; i < MAX_THREAD_COUNT; i++) {
        if (pthread_create(&threads[i], NULL, (void *)start_http_task,
                           &(http_thread_args_t){.tasks_queue_count = &tasks_count, .tasks_queue = &tasks})) {
            return EXIT_FAILURE;
        }
    }

    // TODO implement a way to close all fd even when doing SIGINT
    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_len;
        int client_fd;

        client_fd = accept(fd, &client_address, &client_len);
        if (client_fd == -1) {
            printf("Failed to connect to client\n");
            continue;
        }

        http_task_t task = {.handle = &handle_http_request, .arg1 = client_fd, .arg2 = public_path};

        // TODO we should check if tasks has enought space to store in queue
        enqueue_http_task(&tasks, &tasks_count, task);
    }

    for (int i = 0; i < MAX_THREAD_COUNT; i++) {
        if (pthread_join(threads[i], NULL)) {
            printf("Failed to join threads\n");
            return EXIT_FAILURE;
        }
    }

    destroy_http_tasks();
    return 0;
}
