#pragma once

#include <pthread.h>
#include <stddef.h>

// Implements a thread pool for http task handling

typedef struct HttpTask {
    void (*handle)(int, char *);
    int arg1;
    char *arg2;
} http_task_t;

typedef struct HttpStartThreadArgs {
    http_task_t **tasks_queue;
    size_t *tasks_queue_count;
} http_thread_args_t;

void *enqueue_http_task(http_task_t **tasks_queue, size_t *tasks_queue_count, http_task_t task);

void *start_http_task(http_thread_args_t *args);

void setup_http_tasks();
void destroy_http_tasks();
