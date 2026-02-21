#include <stdio.h>
/*
    Quick Sort Analysis:
    ────────────────────────────────────────────────────────────────────
    How it works:
        1. Choose a pivot element (last element used here).
        2. Partition: rearrange the array so all elements smaller than
           the pivot come before it, and all greater ones come after.
           The pivot is now in its final sorted position.
        3. Recursively apply the same process to the left and right
           sub-arrays around the pivot.

    Time Complexity:
        - Best Case:    O(n log n)  (pivot always splits evenly)
        - Average Case: O(n log n)
        - Worst Case:   O(n²)       (pivot is always the min or max)

    Space Complexity: O(log n)  (call stack)

    Stability: Unstable
*/

static void swap(int *a, int *b) {
  int temp = *a;
  *a = *b;
  *b = temp;
}

static int partition(int *arr, int low, int high) {
  int pivot = arr[high];
  int i = low - 1;

  for (int j = low; j < high; j++)
    if (arr[j] <= pivot)
      swap(&arr[++i], &arr[j]);

  swap(&arr[i + 1], &arr[high]);
  return i + 1;
}

static void quickSort(int *arr, int low, int high) {
  if (low >= high)
    return;
  int pi = partition(arr, low, high);
  quickSort(arr, low, pi - 1);
  quickSort(arr, pi + 1, high);
}

int main() {
  int arr[] = {9, 3, 7, 1, 8, 2, 6, 4, 5};
  int n = sizeof(arr) / sizeof(arr[0]);

  quickSort(arr, 0, n - 1);

  printf("Quick Sort Result: ");
  for (int i = 0; i < n; i++)
    printf("%d ", arr[i]);
  printf("\n");

  return 0;
}
