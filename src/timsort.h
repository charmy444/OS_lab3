#ifndef TIMSORT_H
#define TIMSORT_H

#include <stddef.h>

void timsort(int *a, int *scratch, size_t count);
void merge_ranges(int *a, int *scratch, size_t left, size_t middle, size_t right);

#endif
