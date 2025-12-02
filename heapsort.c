/*
To compile in WSL Ubuntu:
    gcc -O2 -Wall heapsort_alt.c -o heapsort_alt

To run:
    ./heapsort_alt
*/

#include <stdio.h>

/* Cleaner swap helper */
static inline void swap_int(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

/* Iterative-style heapify (still recursive at the end, but inline logic changed) */
void sift_down(int arr[], int heap_size, int root) {
    while (1) {
        int left  = 2 * root + 1;
        int right = 2 * root + 2;
        int largest = root;

        if (left < heap_size && arr[left] > arr[largest])
            largest = left;

        if (right < heap_size && arr[right] > arr[largest])
            largest = right;

        if (largest == root)
            return;   /* no change needed */

        swap_int(&arr[root], &arr[largest]);
        root = largest;
    }
}

void heap_sort(int arr[], int n) {
    /* Build max heap */
    for (int i = (n / 2) - 1; i >= 0; --i)
        sift_down(arr, n, i);

    /* Extract max one by one */
    for (int end = n - 1; end > 0; --end) {
        swap_int(&arr[0], &arr[end]);   /* Move max to end */
        sift_down(arr, end, 0);         /* Restore heap */
    }
}

int main(void) {
    int arr[] = {20, 18, 5, 15, 3, 2};
    int n = sizeof(arr) / sizeof(arr[0]);

    printf("Original Array : ");
    for (int i = 0; i < n; i++)
        printf("%d ", arr[i]);
    printf("\n");

    heap_sort(arr, n);

    printf("Array after performing heap sort: ");
    for (int i = 0; i < n; i++)
        printf("%d ", arr[i]);
    printf("\n");

    return 0;
}
