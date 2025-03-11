#include "http_thread.h"
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>

// Implements a thread pool for http task handling

// TODO investigate if these should be declared in the main function
pthread_mutex_t task_queue_mutex;
pthread_cond_t task_queue_cond;

void enqueue_http_task(http_task_t **tasks_queue, size_t *tasks_queue_count, http_task_t *task) {
    pthread_mutex_lock(&task_queue_mutex);
    (*tasks_queue)[(*tasks_queue_count)] = *task;
    (*tasks_queue_count)++;
    pthread_mutex_unlock(&task_queue_mutex);
    pthread_cond_signal(&task_queue_cond);
}

void *start_http_task(http_thread_args_t *args) {
    size_t *tasks_queue_count = args->tasks_queue_count;
    http_task_t **tasks_queue = args->tasks_queue;

    http_task_t task;

    while (1) {
        pthread_mutex_lock(&task_queue_mutex);

        while (*tasks_queue_count == 0) {
            pthread_cond_wait(&task_queue_cond, &task_queue_mutex);
        }

        task = (*tasks_queue)[0];

        for (size_t i = 0; i < (*tasks_queue_count) - 1; i++) {
            (*tasks_queue)[i] = (*tasks_queue)[i + 1];
        }
        (*tasks_queue_count)--;

        pthread_mutex_unlock(&task_queue_mutex);

        task.handle(task.arg1, task.arg2);
    }
}

void setup_http_tasks() {
    pthread_cond_init(&task_queue_cond, NULL);
}

void destroy_http_tasks() {
    pthread_cond_destroy(&task_queue_cond);
}
