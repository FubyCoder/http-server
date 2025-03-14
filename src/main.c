#include <errno.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "fs.h"
#include "http/content-type.h"
#include "http/headers.h"
#include "http/parser.h"
#include "http/status.h"
#include "http_thread.h"
#include "str.h"

const int MAX_THREAD_COUNT = 8;

request_t *parse_request(int client_fd) {
    const int CHUNK_SIZE = 512;
    size_t buffer_size = 0;
    size_t buffer_capacity = CHUNK_SIZE;
    char *buffer = calloc(CHUNK_SIZE, 1);

    if (buffer == NULL) {
        return NULL;
    }

    long read_result = -2;

    while (read_result == -2 || read_result == CHUNK_SIZE) {
        read_result = read(client_fd, buffer + buffer_size, CHUNK_SIZE);

        if (read_result == -1) {
            // TODO handle error
            return NULL;
        }

        if (buffer_size + CHUNK_SIZE + 1 >= buffer_capacity) {
            size_t new_capacity = buffer_capacity + CHUNK_SIZE + 1;
            char *new_buffer = realloc(buffer, new_capacity);

            if (new_buffer == NULL) {
                return NULL;
            }

            buffer = new_buffer;
            buffer_capacity = new_capacity;
        }

        buffer_size += read_result;
    }

    buffer[buffer_size] = '\0';

    buffer_size++;

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
        enqueue_http_task(&tasks, &tasks_count, &task);
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
