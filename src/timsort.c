#include "timsort.h"

#include <stdbool.h>
#include <string.h>

typedef struct {
    size_t start;
    size_t length;
} Run;

size_t minrun(size_t n)
{
    size_t remainder = 0;
    while (n >= 64) {
        remainder |= n & 1;
        n >>= 1;
    }
    return n + remainder;
}

size_t natural_run(int *a, size_t start, size_t end)
{
    size_t next = start + 1;
    if (next == end)
        return 1;
    if (a[next] < a[start]) {
        while (++next < end && a[next] < a[next - 1]) {}
        for (size_t i = start, j = next - 1; i < j; ++i, --j) {
            int value = a[i];
            a[i] = a[j];
            a[j] = value;
        }
    } else {
        while (++next < end && a[next] >= a[next - 1]) {}
    }
    return next - start;
}

void binary_insertion(int *a, size_t start, size_t end, size_t sorted)
{
    for (size_t i = start + sorted; i < end; ++i) {
        int value = a[i];
        size_t low = start, high = i;
        while (low < high) {
            size_t middle = low + (high - low) / 2;
            if (value < a[middle])
                high = middle;
            else
                low = middle + 1;
        }
        memmove(a + low + 1, a + low, (i - low) * sizeof(*a));
        a[low] = value;
    }
}

size_t gallop(const int *a, size_t count, int key, bool inclusive)
{
    if (count == 0 || !(inclusive ? a[0] <= key : a[0] < key))
        return 0;
    size_t low = 1, high = 1;
    while (high < count && (inclusive ? a[high] <= key : a[high] < key)) {
        low = high + 1;
        if (high > (count - 1) / 2) {
            high = count;
            break;
        }
        high = high * 2 + 1;
    }
    if (high < count)
        ++high;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        if (inclusive ? a[middle] <= key : a[middle] < key)
            low = middle + 1;
        else
            high = middle;
    }
    return low;
}

void merge_ranges(int *a, int *scratch, size_t left, size_t middle, size_t right)
{
    if (left == middle || middle == right || a[middle - 1] <= a[middle])
        return;
    memcpy(scratch + left, a + left, (right - left) * sizeof(*a));
    size_t i = left, j = middle, out = left;
    unsigned left_wins = 0, right_wins = 0;
    while (i < middle && j < right) {
        if (scratch[i] <= scratch[j]) {
            a[out++] = scratch[i++];
            ++left_wins;
            right_wins = 0;
            if (left_wins >= 7 && i < middle) {
                size_t block = gallop(scratch + i, middle - i, scratch[j], true);
                memcpy(a + out, scratch + i, block * sizeof(*a));
                i += block;
                out += block;
                left_wins = 0;
            }
        } else {
            a[out++] = scratch[j++];
            ++right_wins;
            left_wins = 0;
            if (right_wins >= 7 && j < right) {
                size_t block = gallop(scratch + j, right - j, scratch[i], false);
                memcpy(a + out, scratch + j, block * sizeof(*a));
                j += block;
                out += block;
                right_wins = 0;
            }
        }
    }
    if (i < middle)
        memcpy(a + out, scratch + i, (middle - i) * sizeof(*a));
    else if (j < right)
        memcpy(a + out, scratch + j, (right - j) * sizeof(*a));
}

void merge_at(int *a, int *scratch, Run *runs, size_t *used, size_t at)
{
    merge_ranges(a, scratch, runs[at].start, runs[at + 1].start,
                 runs[at + 1].start + runs[at + 1].length);
    runs[at].length += runs[at + 1].length;
    for (size_t i = at + 1; i + 1 < *used; ++i)
        runs[i] = runs[i + 1];
    --*used;
}

void timsort(int *a, int *scratch, size_t count)
{
    if (count < 2)
        return;
    Run runs[sizeof(size_t) * 8 * 2];
    size_t used = 0, start = 0, minimum = minrun(count);
    while (start < count) {
        size_t length = natural_run(a, start, count);
        size_t target = count - start < minimum ? count - start : minimum;
        if (length < target) {
            binary_insertion(a, start, start + target, length);
            length = target;
        }
        runs[used++] = (Run){start, length};
        start += length;
        while (used > 1) {
            size_t n = used - 2;
            if ((n > 0 && runs[n - 1].length <= runs[n].length + runs[n + 1].length) ||
                (n > 1 && runs[n - 2].length <= runs[n - 1].length + runs[n].length)) {
                if (runs[n - 1].length < runs[n + 1].length)
                    --n;
            } else if (runs[n].length > runs[n + 1].length) {
                break;
            }
            merge_at(a, scratch, runs, &used, n);
        }
    }
    while (used > 1) {
        size_t n = used - 2;
        if (n > 0 && runs[n - 1].length < runs[n + 1].length)
            --n;
        merge_at(a, scratch, runs, &used, n);
    }
}
