#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

/* Standard swap */
static inline void swap_int(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

/* Optimized bubble sort */
void bubbleSort(int arr[], int n) {
    for (int i = 0; i < n - 1; ++i) {
        bool swapped = false;
        for (int j = 0; j < n - i - 1; ++j) {
            if (arr[j] > arr[j + 1]) {
                swap_int(&arr[j], &arr[j + 1]);
                swapped = true;
            }
        }
        if (!swapped) break;
    }
}

/* Fill an array with random values 1–100 */
static inline void fill_random(int arr[], int n) {
    for (int i = 0; i < n; ++i) {
        arr[i] = (rand() % 100) + 1;
    }
}

/* Run `iterations` times: fill array -> sort array */
double benchmark_size(int n, int iterations) {
    int *buf = malloc(sizeof(int) * n);
    if (!buf) {
        fprintf(stderr, "Memory allocation failed for size %d\n", n);
        exit(EXIT_FAILURE);
    }

    clock_t start = clock();

    for (int it = 0; it < iterations; ++it) {
        fill_random(buf, n);
        bubbleSort(buf, n);
    }

    clock_t end = clock();
    free(buf);

    return (double)(end - start) / CLOCKS_PER_SEC;
}

int main(void) {

    srand((unsigned)time(NULL));  // Seed RNG once

    const int sizes[] = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
    const int count = sizeof(sizes) / sizeof(sizes[0]);
    const int iterations = 10000;

    printf("\n");  // Format matches original output

    for (int i = 0; i < count; ++i) {
        int n = sizes[i];
        double cpu_time_used = benchmark_size(n, iterations);
        printf("CPU time used: %f seconds for %d length array\n",
               cpu_time_used, n);
    }

    return 0;
}
