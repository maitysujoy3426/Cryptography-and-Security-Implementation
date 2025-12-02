/*
Sorting benchmark: Bubble, Quick, Merge, Heap
Writes CSV: sorting_results.csv

Compile (WSL Ubuntu):
    gcc -O2 -Wall sorting_results.c -o sorting_results -lm

Run:
    ./sorting_results

CSV columns:
    algorithm,size,runs,cpu_time_seconds,avg_comparisons,avg_swaps

Notes:
 - Uses rand() seeded by current time.
 - Runs = 1000 (change RUNS to adjust).
 - Array sizes 100..1000 step 100 (change MIN_SIZE/MAX_SIZE/STEP if desired).
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

#define MIN_SIZE 100
#define MAX_SIZE 1000
#define STEP 100
#define RUNS 1000

typedef struct {
    long long comparisons;
    long long swaps;
} Stats;

/* ---------- Utility ---------- */

static inline void swap_int(int *a, int *b, Stats *s) {
    int t = *a;
    *a = *b;
    *b = t;
    s->swaps++;
}

static void fill_random(int arr[], int n) {
    for (int i = 0; i < n; ++i) arr[i] = rand();
}

/* ---------- Bubble Sort (instrumented) ---------- */
void bubble_sort_inst(int arr[], int n, Stats *s) {
    for (int i = 0; i < n - 1; ++i) {
        int swapped = 0;
        for (int j = 0; j < n - i - 1; ++j) {
            s->comparisons++;
            if (arr[j] > arr[j + 1]) {
                swap_int(&arr[j], &arr[j + 1], s);
                swapped = 1;
            }
        }
        if (!swapped) break;
    }
}

/* ---------- Quick Sort (instrumented) ---------- */
static void swap_q(int *a, int *b, Stats *s) {
    int t = *a; *a = *b; *b = t; s->swaps++;
}

static int partition_q(int arr[], int low, int high, Stats *s) {
    int pivot = arr[high];
    int i = low - 1;
    for (int j = low; j <= high - 1; ++j) {
        s->comparisons++;
        if (arr[j] < pivot) {
            ++i;
            swap_q(&arr[i], &arr[j], s);
        }
    }
    swap_q(&arr[i + 1], &arr[high], s);
    return i + 1;
}

static void quick_sort_rec(int arr[], int low, int high, Stats *s) {
    if (low < high) {
        int pi = partition_q(arr, low, high, s);
        quick_sort_rec(arr, low, pi - 1, s);
        quick_sort_rec(arr, pi + 1, high, s);
    }
}

void quick_sort_inst(int arr[], int n, Stats *s) {
    quick_sort_rec(arr, 0, n - 1, s);
}

/* ---------- Merge Sort (instrumented) ---------- */
static void merge_inst(int arr[], int l, int m, int r, Stats *s) {
    int n1 = m - l + 1;
    int n2 = r - m;
    int *L = malloc(n1 * sizeof(int));
    int *R = malloc(n2 * sizeof(int));
    if (!L || !R) { free(L); free(R); return; }

    for (int i = 0; i < n1; ++i) L[i] = arr[l + i];
    for (int j = 0; j < n2; ++j) R[j] = arr[m + 1 + j];

    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        s->comparisons++;
        if (L[i] <= R[j]) {
            arr[k++] = L[i++];
        } else {
            arr[k++] = R[j++];
        }
        s->swaps++; /* count an assignment as a move */
    }
    while (i < n1) { arr[k++] = L[i++]; s->swaps++; }
    while (j < n2) { arr[k++] = R[j++]; s->swaps++; }

    free(L); free(R);
}

static void merge_sort_rec(int arr[], int l, int r, Stats *s) {
    if (l < r) {
        int m = l + (r - l) / 2;
        merge_sort_rec(arr, l, m, s);
        merge_sort_rec(arr, m + 1, r, s);
        merge_inst(arr, l, m, r, s);
    }
}

void merge_sort_inst(int arr[], int n, Stats *s) {
    merge_sort_rec(arr, 0, n - 1, s);
}

/* ---------- Heap Sort (instrumented) ---------- */
static void sift_down_inst(int arr[], int heap_n, int root, Stats *s) {
    while (1) {
        int left = 2 * root + 1;
        int right = 2 * root + 2;
        int largest = root;

        if (left < heap_n) {
            s->comparisons++;
            if (arr[left] > arr[largest]) largest = left;
        }
        if (right < heap_n) {
            s->comparisons++;
            if (arr[right] > arr[largest]) largest = right;
        }
        if (largest == root) return;
        swap_q(&arr[root], &arr[largest], s);
        root = largest;
    }
}

void heap_sort_inst(int arr[], int n, Stats *s) {
    for (int i = n / 2 - 1; i >= 0; --i) sift_down_inst(arr, n, i, s);
    for (int end = n - 1; end > 0; --end) {
        swap_q(&arr[0], &arr[end], s);
        sift_down_inst(arr, end, 0, s);
    }
}

/* ---------- Runner & CSV ---------- */

typedef void (*SortFn)(int[], int, Stats *);

void run_and_record(FILE *csv, const char *name, SortFn fn, int size) {
    int *buffer = malloc(sizeof(int) * size);
    int *work = malloc(sizeof(int) * size);
    if (!buffer || !work) {
        fprintf(stderr, "Allocation failed for size %d\n", size);
        free(buffer); free(work);
        return;
    }

    long long total_comps = 0;
    long long total_swaps = 0;

    clock_t t0 = clock();
    for (int r = 0; r < RUNS; ++r) {
        fill_random(buffer, size);
        memcpy(work, buffer, sizeof(int) * size);

        Stats s = {0,0};
        fn(work, size, &s);

        total_comps += s.comparisons;
        total_swaps += s.swaps;
    }
    clock_t t1 = clock();

    double cpu_time = (double)(t1 - t0) / CLOCKS_PER_SEC;
    double avg_comps = total_comps / (double)RUNS;
    double avg_swaps = total_swaps / (double)RUNS;

    fprintf(csv, "%s,%d,%d,%.6f,%.2f,%.2f\n",
            name, size, RUNS, cpu_time, avg_comps, avg_swaps);

    free(buffer); free(work);
}

int main(void) {
    srand((unsigned)time(NULL));

    FILE *csv = fopen("sorting_results.csv", "w");
    if (!csv) {
        fprintf(stderr, "Cannot open sorting_results.csv for writing\n");
        return 1;
    }

    fprintf(csv, "algorithm,size,runs,cpu_time_seconds,avg_comparisons,avg_swaps\n");

    /* For each size, run each sorting algorithm */
    for (int size = MIN_SIZE; size <= MAX_SIZE; size += STEP) {
        run_and_record(csv, "Bubble", bubble_sort_inst, size);
        run_and_record(csv, "Quick", quick_sort_inst, size);
        run_and_record(csv, "Merge", merge_sort_inst, size);
        run_and_record(csv, "Heap", heap_sort_inst, size);
        /* flush per-size so partial results are saved if interrupted */
        fflush(csv);
    }

    fclose(csv);
    printf("Results written to sorting_results.csv\n");
    return 0;
}
