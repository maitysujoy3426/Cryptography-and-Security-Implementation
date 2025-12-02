#include <stdio.h>

int i, j;
void bubblesort(int arr[], int n)
{
    for (i = 0; i < n - 1; i++)
    {
        for (j = 0; j < n - i - 1; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}
void printarray(int arr[], int n)
{
    for (i = 0; i < n; i++)
    {
        printf(" %d ", arr[i]);
        printf("\n");
    }
}
int main()
{
    int arr[] = {
        1,
        5,
        2,
        8,
        3,
        4,
        5,
        2,
        2,
        2,
        5,
        8,
        99,
        2,
        5,
        3,
        1,
        2,
        2,
        2,
        25,
        4,
        5,
        8,
        4,
    };
    int n = sizeof(arr) / sizeof(arr[0]);

    printf("The original array is: \n");
    printarray(arr, n);

    bubblesort(arr, n);
    printf("The sorted array is: \n");
    printarray(arr, n);
}
