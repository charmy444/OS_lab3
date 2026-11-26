#include "parallel.h"
#include "timsort.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int *values, *scratch;
    size_t left, middle, right;
    bool merge;
} Task;

void *worker(void *argument)
{
    Task *task = argument;
    if (task->merge)
        merge_ranges(task->values, task->scratch,
                     task->left, task->middle, task->right);
    else
        timsort(task->values + task->left, task->scratch + task->left,
                task->right - task->left);
    return NULL;
}

int run_stage(Task *tasks, size_t count, pthread_t *threads)
{
    size_t created = 0;
    int result = 0;
    for (size_t i = 1; i < count; ++i) {
        int error = pthread_create(&threads[created], NULL, worker, &tasks[i]);
        if (error != 0) {
            fprintf(stderr, "Не удалось создать поток: %s\n", strerror(error));
            result = -1;
            break;
        }
        ++created;
    }
    if (result == 0)
        worker(&tasks[0]);
    for (size_t i = 0; i < created; ++i) {
        int error = pthread_join(threads[i], NULL);
        if (error != 0) {
            fprintf(stderr, "Не удалось дождаться завершения потока: %s\n", strerror(error));
            exit(EXIT_FAILURE);
        }
    }
    return result;
}

int parallel_timsort(int *values, size_t count, size_t limit)
{
    if (count < 2)
        return 0;
    size_t parts = limit < count ? limit : count;
    if (count > SIZE_MAX / sizeof(int) || parts > SIZE_MAX / sizeof(Task) ||
        parts > SIZE_MAX / sizeof(pthread_t)) {
        fprintf(stderr, "Слишком большой массив или лимит потоков\n");
        return -1;
    }
    int *scratch = malloc(count * sizeof(*scratch));
    Task *tasks = malloc(parts * sizeof(*tasks));
    pthread_t *threads = malloc(parts * sizeof(*threads));
    int result = -1;
    if (scratch == NULL || tasks == NULL || threads == NULL) {
        fprintf(stderr, "Не удалось выделить вспомогательную память для сортировки\n");
        goto cleanup;
    }
    size_t begin = 0;
    for (size_t i = 0; i < parts; ++i) {
        size_t length = count / parts + (i < count % parts);
        tasks[i] = (Task){values, scratch, begin, begin, begin + length, false};
        begin += length;
    }
    result = run_stage(tasks, parts, threads);
    while (result == 0 && parts > 1) {
        size_t pairs = parts / 2;
        Task unpaired = tasks[parts - 1];
        for (size_t i = 0; i < pairs; ++i) {
            Task left = tasks[2 * i], right = tasks[2 * i + 1];
            tasks[i] = (Task){values, scratch, left.left, right.left, right.right, true};
        }
        result = run_stage(tasks, pairs, threads);
        if (parts % 2 != 0)
            tasks[pairs] = unpaired;
        parts = pairs + parts % 2;
    }
cleanup:
    free(threads);
    free(tasks);
    free(scratch);
    return result;
}
