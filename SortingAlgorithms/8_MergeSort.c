#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
    Merge Sort Analysis:
    ────────────────────────────────────────────────────────────────────
    How it works:
        1. Recursively split the array in half until each sub-array
           has only one element (a single element is trivially sorted).
        2. Merge pairs of sorted sub-arrays back together by comparing
           their front elements and copying the smaller one each time.
        3. Repeat until the entire array is merged into one sorted array.

    Time Complexity:
        - Best Case:    O(n log n)
        - Average Case: O(n log n)
        - Worst Case:   O(n log n)

    Space Complexity: O(n)  (auxiliary buffer for merging)

    Stability: Stable
*/

static void merge(int *arr, int left, int mid, int right) {
  int lenL = mid - left + 1;
  int lenR = right - mid;

  int *L = (int *)malloc(lenL * sizeof(int));
  int *R = (int *)malloc(lenR * sizeof(int));

  memcpy(L, arr + left, lenL * sizeof(int));
  memcpy(R, arr + mid + 1, lenR * sizeof(int));

  int i = 0, j = 0, k = left;
  while (i < lenL && j < lenR)
    arr[k++] = (L[i] <= R[j]) ? L[i++] : R[j++];

  while (i < lenL)
    arr[k++] = L[i++];
  while (j < lenR)
    arr[k++] = R[j++];

  free(L);
  free(R);
}

static void mergeSort(int *arr, int left, int right) {
  if (left >= right)
    return;
  int mid = left + (right - left) / 2;
  mergeSort(arr, left, mid);
  mergeSort(arr, mid + 1, right);
  merge(arr, left, mid, right);
}

int main() {
  int arr[] = {9, 3, 7, 1, 8, 2, 6, 4, 5};
  int n = sizeof(arr) / sizeof(arr[0]);

  mergeSort(arr, 0, n - 1);

  printf("Merge Sort Result: ");
  for (int i = 0; i < n; i++)
    printf("%d ", arr[i]);
  printf("\n");

  return 0;
}