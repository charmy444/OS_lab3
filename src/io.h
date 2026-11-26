#ifndef IO_H
#define IO_H

#include <stddef.h>

int read_thread_limit(int argc, char **argv, size_t *limit);
int read_array(int **values, size_t *count);
int print_array(const int *values, size_t count);

#endif
