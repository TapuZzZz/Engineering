#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
    Radix Sort Analysis:
    ────────────────────────────────────────────────────────────────────
    How it works:
        1. Find the maximum value to know how many digit passes are needed.
        2. For each digit position (1s, 10s, 100s, ...):
             a. Run a stable Counting Sort on that digit alone (base 10).
             b. Because each pass is stable, relative order from prior
                passes is preserved, yielding a fully sorted array after
                the last digit pass.

    Time Complexity:
        - Best Case:    O(d * n)
        - Average Case: O(d * n)
        - Worst Case:   O(d * n)
          where d = number of digits in the maximum value, n = input size

    Space Complexity: O(n + b)  where b = base (10)

    Stability: Stable
*/

static void countByDigit(int *arr, int n, int exp) {
  int *output = (int *)malloc(n * sizeof(int));
  int count[10] = {0};

  for (int i = 0; i < n; i++)
    count[(arr[i] / exp) % 10]++;

  for (int i = 1; i < 10; i++)
    count[i] += count[i - 1];

  for (int i = n - 1; i >= 0; i--)
    output[--count[(arr[i] / exp) % 10]] = arr[i];

  memcpy(arr, output, n * sizeof(int));
  free(output);
}

int main() {
  int arr[] = {170, 45, 75, 90, 802, 24, 2, 66};
  int n = sizeof(arr) / sizeof(arr[0]);

  int max = arr[0];
  for (int i = 1; i < n; i++)
    if (arr[i] > max)
      max = arr[i];

  for (int exp = 1; max / exp > 0; exp *= 10)
    countByDigit(arr, n, exp);

  printf("Radix Sort Result: ");
  for (int i = 0; i < n; i++)
    printf("%d ", arr[i]);
  printf("\n");

  return 0;
}