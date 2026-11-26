#include "io.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool read_token(char *token, size_t capacity)
{
    int ch = getchar();
    while (ch != EOF && isspace(ch))
        ch = getchar();

    size_t length = 0;
    while (ch != EOF && !isspace(ch)) {
        if (ch == '\0' || length + 1 >= capacity)
            return false;
        token[length] = (char)ch;
        ++length;
        ch = getchar();
    }
    if (length == 0 || ferror(stdin))
        return false;

    token[length] = '\0';
    return true;
}

int parse_number(const char *text, size_t maximum, size_t *value)
{
    if (text[0] < '0' || text[0] > '9')
        return -1;

    char *end;
    errno = 0;
    unsigned long long number = strtoull(text, &end, 10);
    if (errno != 0 || *end != '\0' || number > maximum)
        return -1;

    *value = (size_t)number;
    return 0;
}

int read_thread_limit(int argc, char **argv, size_t *limit)
{
    if (argc != 3 || strcmp(argv[1], "--threads") != 0 ||
        parse_number(argv[2], SIZE_MAX, limit) != 0 || *limit == 0) {
        fprintf(stderr, "Запуск: %s --threads N\nВвод: длина массива, затем целые неотриц числа.\n", argv[0]);
        return -1;
    }
    return 0;
}

int read_array(int **values, size_t *count)
{
    char token[128];
    if (!read_token(token, sizeof(token)) ||
        parse_number(token, INT_MAX, count) != 0 ||
        *count > SIZE_MAX / sizeof(**values)) {
        fprintf(stderr, "Ожидается длина массива от 0 до %d\n", INT_MAX);
        return -1;
    }
    if (*count == 0) {
        *values = NULL;
        return 0;
    }
    *values = malloc(*count * sizeof(**values));
    if (*values == NULL) {
        perror("Не удалось выделить память для массива");
        return -1;
    }
    for (size_t i = 0; i < *count; ++i) {
        if (!read_token(token, sizeof(token))) {
            fprintf(stderr, "Не удалось прочитать элемент №%zu\n", i + 1);
            return -1;
        }
        char *end;
        errno = 0;
        long value = strtol(token, &end, 10);
        if (errno != 0 || *end != '\0' || value < INT_MIN || value > INT_MAX) {
            fprintf(stderr, "Элемент №%zu должен быть целым числом от %d до %d\n", i + 1, INT_MIN, INT_MAX);
            return -1;
        }
        (*values)[i] = (int)value;
    }
    return 0;
}

int print_array(const int *values, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        if (printf("%s%d", i == 0 ? "" : " ", values[i]) < 0) {
            perror("Не удалось вывести массив");
            return -1;
        }
    }
    if (putchar('\n') == EOF || fflush(stdout) == EOF) {
        perror("Не удалось завершить вывод массива");
        return -1;
    }
    return 0;
}
