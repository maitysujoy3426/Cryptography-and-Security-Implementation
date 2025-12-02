/*
Refactored sorting benchmark for WSL Ubuntu.

To compile:
    gcc -O2 -Wall sort_bench_refactored.c -o sort_bench_refactored -lm

To run:
    ./sort_bench_refactored

Notes:
- Produces console output similar to the original program and writes "benchmark_results.csv".
- Uses a placeholder CPU frequency of 3e9 Hz for the clock_cycles column (same as original).
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

/* ---- Types & prototypes ---- */

typedef void (*SortFn)(int *arr, int n, long long *comparisons, long long *swaps);

typedef struct {
    const char *name;
    SortFn fn;
} SortEntry;

static int cmp_ll_for_qsort(const void *a, const void *b);
static double median_ll(long long *vals, int n);
static void run_benchmark(const char *name, SortFn fn, int size, int runs, double complexity, FILE *csv);
static void write_csv_line(FILE *csv, const char *name, int size, int runs, double cpu_time,
                           double clock_cycles, double avg_comps, double avg_swaps, double const_value);

/* ---- Utility functions ---- */

static int cmp_ll_for_qsort(const void *a, const void *b) {
    long long va = *(const long long *)a;
    long long vb = *(const long long *)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

static double median_ll(long long *vals, int n) {
    /* sort a copy so original array order not required by caller */
    long long *buf = malloc(sizeof(long long) * n);
    if (!buf) return 0.0;
    for (int i = 0; i < n; ++i) buf[i] = vals[i];
    qsort(buf, n, sizeof(long long), cmp_ll_for_qsort);
    double med;
    if (n % 2 == 0)
        med = (buf[n/2 - 1] + buf[n/2]) / 2.0;
    else
        med = buf[n/2];
    free(buf);
    return med;
}

/* ---- Sorting implementations (instrumented) ---- */

/* Bubble Sort */
static void bubble_sort_inst(int arr[], int n, long long *comparisons, long long *swaps) {
    for (int i = 0; i < n - 1; ++i) {
        bool swapped = false;
        for (int j = 0; j < n - i - 1; ++j) {
            (*comparisons)++;
            if (arr[j] > arr[j + 1]) {
                int tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
                (*swaps)++;
                swapped = true;
            }
        }
        if (!swapped) break;
    }
}

/* Heap Sort helper */
static void heapify_inst(int arr[], int n, int i, long long *comparisons, long long *swaps) {
    int largest = i;
    int l = 2 * i + 1;
    int r = 2 * i + 2;

    if (l < n) {
        (*comparisons)++;
        if (arr[l] > arr[largest]) largest = l;
    }
    if (r < n) {
        (*comparisons)++;
        if (arr[r] > arr[largest]) largest = r;
    }
    if (largest != i) {
        int tmp = arr[i];
        arr[i] = arr[largest];
        arr[largest] = tmp;
        (*swaps)++;
        heapify_inst(arr, n, largest, comparisons, swaps);
    }
}

static void heap_sort_inst(int arr[], int n, long long *comparisons, long long *swaps) {
    for (int i = n / 2 - 1; i >= 0; --i)
        heapify_inst(arr, n, i, comparisons, swaps);
    for (int i = n - 1; i > 0; --i) {
        int tmp = arr[0];
        arr[0] = arr[i];
        arr[i] = tmp;
        (*swaps)++;
        heapify_inst(arr, i, 0, comparisons, swaps);
    }
}

/* Merge Sort helpers */
static void merge_inst(int arr[], int l, int m, int r, long long *comparisons, long long *swaps) {
    int n1 = m - l + 1;
    int n2 = r - m;
    int *L = malloc(n1 * sizeof(int));
    int *R = malloc(n2 * sizeof(int));
    if (!L || !R) { free(L); free(R); return; }

    for (int i = 0; i < n1; ++i) L[i] = arr[l + i];
    for (int j = 0; j < n2; ++j) R[j] = arr[m + 1 + j];

    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        (*comparisons)++;
        if (L[i] <= R[j]) {
            arr[k++] = L[i++];
        } else {
            arr[k++] = R[j++];
        }
        (*swaps)++;
    }
    while (i < n1) { arr[k++] = L[i++]; (*swaps)++; }
    while (j < n2) { arr[k++] = R[j++]; (*swaps)++; }

    free(L);
    free(R);
}

static void merge_sort_rec(int arr[], int l, int r, long long *comparisons, long long *swaps) {
    if (l < r) {
        int m = l + (r - l) / 2;
        merge_sort_rec(arr, l, m, comparisons, swaps);
        merge_sort_rec(arr, m + 1, r, comparisons, swaps);
        merge_inst(arr, l, m, r, comparisons, swaps);
    }
}

static void merge_sort_inst(int arr[], int n, long long *comparisons, long long *swaps) {
    merge_sort_rec(arr, 0, n - 1, comparisons, swaps);
}

/* Quick Sort helpers */
static void swap_quick_inst(int *a, int *b, long long *swaps) {
    int t = *a; *a = *b; *b = t; (*swaps)++;
}

static int partition_inst(int arr[], int low, int high, long long *comparisons, long long *swaps) {
    int pivot = arr[high];
    int i = low - 1;
    for (int j = low; j <= high - 1; ++j) {
        (*comparisons)++;
        if (arr[j] < pivot) {
            ++i;
            swap_quick_inst(&arr[i], &arr[j], swaps);
        }
    }
    swap_quick_inst(&arr[i + 1], &arr[high], swaps);
    return i + 1;
}

static void quick_sort_rec(int arr[], int low, int high, long long *comparisons, long long *swaps) {
    if (low < high) {
        int pi = partition_inst(arr, low, high, comparisons, swaps);
        quick_sort_rec(arr, low, pi - 1, comparisons, swaps);
        quick_sort_rec(arr, pi + 1, high, comparisons, swaps);
    }
}

static void quick_sort_inst(int arr[], int n, long long *comparisons, long long *swaps) {
    quick_sort_rec(arr, 0, n - 1, comparisons, swaps);
}

/* ---- Benchmark runner ---- */

static void run_benchmark(const char *name, SortFn fn, int size, int runs, double complexity, FILE *csv) {
    int *workspace = malloc(sizeof(int) * size);
    if (!workspace) {
        fprintf(stderr, "Allocation failed for size %d\n", size);
        return;
    }

    long long *cmp_runs = malloc(sizeof(long long) * runs);
    long long *swp_runs = malloc(sizeof(long long) * runs);
    if (!cmp_runs || !swp_runs) {
        fprintf(stderr, "Allocation failed for run arrays\n");
        free(workspace); free(cmp_runs); free(swp_runs);
        return;
    }

    long long total_cmp = 0, total_swp = 0;

    clock_t t0 = clock();
    for (int r = 0; r < runs; ++r) {
        for (int i = 0; i < size; ++i) workspace[i] = (rand() % 100) + 1;

        long long comps = 0, swaps = 0;
        fn(workspace, size, &comps, &swaps);

        cmp_runs[r] = comps;
        swp_runs[r] = swaps;
        total_cmp += comps;
        total_swp += swaps;
    }
    clock_t t1 = clock();

    double cpu_time = (double)(t1 - t0) / CLOCKS_PER_SEC;
    double median_cmp = median_ll(cmp_runs, runs);
    double median_swp = median_ll(swp_runs, runs);
    double const_sort = (double)total_cmp / complexity;

    long long min_cmp = cmp_runs[0], max_cmp = cmp_runs[0];
    long long min_swp = swp_runs[0], max_swp = swp_runs[0];
    for (int i = 1; i < runs; ++i) {
        if (cmp_runs[i] < min_cmp) min_cmp = cmp_runs[i];
        if (cmp_runs[i] > max_cmp) max_cmp = cmp_runs[i];
        if (swp_runs[i] < min_swp) min_swp = swp_runs[i];
        if (swp_runs[i] > max_swp) max_swp = swp_runs[i];
    }

    printf("\n--- %s ---\n", name);
    printf("Array size: %d, Runs: %d\n", size, runs);
    printf("CPU time: %.4f seconds\n", cpu_time);
    printf("Avg comparisons: %.2f\n", (double)total_cmp / runs);
    printf("Min comparisons: %lld, Max comparisons: %lld, Median: %.2f\n", min_cmp, max_cmp, median_cmp);
    printf("Dividing number of comparisons by complexity: %.2f\n", const_sort);
    printf("Avg swaps: %.2f\n", (double)total_swp / runs);
    printf("Min swaps: %lld, Max swaps: %lld, Median: %.2f\n", min_swp, max_swp, median_swp);

    /* clock cycles placeholder using same 3e9 Hz as original */
    double clock_speed_hz = 3e9;
    double clock_cycles = cpu_time * clock_speed_hz;

    write_csv_line(csv, name, size, runs, cpu_time, clock_cycles,
                   (double)total_cmp / runs, (double)total_swp / runs, const_sort);

    free(workspace);
    free(cmp_runs);
    free(swp_runs);
}

static void write_csv_line(FILE *csv, const char *name, int size, int runs, double cpu_time,
                           double clock_cycles, double avg_comps, double avg_swaps, double const_value) {
    fprintf(csv, "%s,%d,%d,%.6f,%.0f,%.2f,%.2f,%.2f\n",
            name, size, runs, cpu_time, clock_cycles, avg_comps, avg_swaps, const_value);
}

/* ---- Main ---- */

int main(void) {
    srand((unsigned)time(NULL));

    const SortEntry sorts[] = {
        {"Bubble Sort", bubble_sort_inst},
        {"Quick Sort", quick_sort_inst},
        {"Merge Sort", merge_sort_inst},
        {"Heap Sort", heap_sort_inst}
    };
    const int sort_count = sizeof(sorts) / sizeof(sorts[0]);

    const int runs = 10000;

    FILE *csv = fopen("benchmark_results.csv", "w");
    if (!csv) {
        fprintf(stderr, "Error opening benchmark_results.csv for writing\n");
        return EXIT_FAILURE;
    }
    fprintf(csv, "sort,size,runs,cpu_time,clock_cycles,avg_comps,avg_swaps,constant_value\n");

    for (int size = 100; size <= 1000; size += 100) {
        for (int si = 0; si < sort_count; ++si) {
            double complexity;
            if (si == 0) { /* Bubble: n^2 */
                complexity = pow((double)size, 2.0);
            } else { /* Quick, Merge, Heap: n * log n */
                complexity = (double)size * log((double)size);
            }
            run_benchmark(sorts[si].name, sorts[si].fn, size, runs, complexity, csv);
        }
    }

    fclose(csv);
    return 0;
}
