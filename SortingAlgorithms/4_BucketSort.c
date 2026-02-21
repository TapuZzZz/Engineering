#include <stdio.h>
#include <stdlib.h>
/*
    Bucket Sort Analysis:
    ────────────────────────────────────────────────────────────────────
    How it works:
        1. Divide the input range into n equally-sized buckets.
        2. Scatter each element into its corresponding bucket.
        3. Sort each bucket individually (Insertion Sort used here).
        4. Concatenate all buckets back into the original array.

    Time Complexity:
        - Best Case:    O(n + k)  (uniform distribution)
        - Average Case: O(n + k)
        - Worst Case:   O(n²)     (all elements fall into one bucket)
          where k = number of buckets

    Space Complexity: O(n + k)

    Stability: Stable (when the per-bucket sort is stable)

    Note: Works best when input is uniformly distributed over a
          known range. Used here with integers; float version would
          normalise values to [0, 1).
*/

static void insertionSort(int *a, int len) {
  for (int i = 1; i < len; i++) {
    int key = a[i], j = i - 1;
    while (j >= 0 && a[j] > key) {
      a[j + 1] = a[j];
      j--;
    }
    a[j + 1] = key;
  }
}

int main() {
  int arr[] = {9, 3, 7, 1, 8, 2, 6, 4, 5};
  int n = sizeof(arr) / sizeof(arr[0]);

  int min = arr[0], max = arr[0];
  for (int i = 1; i < n; i++) {
    if (arr[i] < min)
      min = arr[i];
    if (arr[i] > max)
      max = arr[i];
  }

  int **buckets = (int **)calloc(n, sizeof(int *));
  int *counts = (int *)calloc(n, sizeof(int));
  int *capacity = (int *)calloc(n, sizeof(int));

  for (int i = 0; i < n; i++) {
    capacity[i] = 4;
    buckets[i] = (int *)malloc(capacity[i] * sizeof(int));
  }

  int range = max - min + 1;
  for (int i = 0; i < n; i++) {
    int idx = (int)((long long)(arr[i] - min) * n / range);
    if (idx >= n)
      idx = n - 1;

    if (counts[idx] == capacity[idx]) {
      capacity[idx] *= 2;
      buckets[idx] = (int *)realloc(buckets[idx], capacity[idx] * sizeof(int));
    }
    buckets[idx][counts[idx]++] = arr[i];
  }

  int pos = 0;
  for (int i = 0; i < n; i++) {
    insertionSort(buckets[i], counts[i]);
    for (int j = 0; j < counts[i]; j++)
      arr[pos++] = buckets[i][j];
    free(buckets[i]);
  }

  free(buckets);
  free(counts);
  free(capacity);

  printf("Bucket Sort Result: ");
  for (int i = 0; i < n; i++)
    printf("%d ", arr[i]);
  printf("\n");

  return 0;
}