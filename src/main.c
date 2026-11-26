#define _POSIX_C_SOURCE 200809L
#include "io.h"
#include "parallel.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

bool is_sorted(const int *values, size_t count)
{
    for (size_t i = 1; i < count; ++i) {
        if (values[i - 1] > values[i]) {
            return false;
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    size_t limit;
    if (read_thread_limit(argc, argv, &limit) != 0)
        return EXIT_FAILURE;

    int *values = NULL;
    size_t count = 0;
    int result = EXIT_FAILURE;
    if (read_array(&values, &count) != 0)
        goto cleanup;

    struct timespec before, after;
    if (clock_gettime(CLOCK_MONOTONIC, &before) == -1) {
        perror("Не удалось получить показание часов");
        goto cleanup;
    }
    if (parallel_timsort(values, count, limit) != 0)
        goto cleanup;
    if (clock_gettime(CLOCK_MONOTONIC, &after) == -1) {
        perror("Не удалось получить показание часов");
        goto cleanup;
    }
    double seconds = (double)(after.tv_sec - before.tv_sec) +
                     (double)(after.tv_nsec - before.tv_nsec) / 1e9;

    if (!is_sorted(values, count)) {
        fprintf(stderr, "Проверка не пройдена: массив не отсортирован\n");
        goto cleanup;
    }
    if (print_array(values, count) != 0)
        goto cleanup;
    if (fprintf(stderr, "Время сортировки: %.9f с. Массив отсортирован.\n", seconds) < 0 ||
        fflush(stderr) == EOF)
        goto cleanup;
    result = EXIT_SUCCESS;
cleanup:
    free(values);
    return result;
}
