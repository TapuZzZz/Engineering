#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
    Counting Sort Analysis:
    ────────────────────────────────────────────────────────────────────
    How it works:
        1. Find the range [min, max] of the input.
        2. Allocate a count array of size (max - min + 1) and tally
           how many times each value appears.
        3. Transform counts into prefix sums so each entry holds the
           starting output index for that value.
        4. Iterate the input in reverse (to preserve stability) and
           place each element at its correct position in the output,
           decrementing the corresponding count after each placement.
        5. Copy the output back to the original array.

    Time Complexity:
        - Best Case:    O(n + range)
        - Average Case: O(n + range)
        - Worst Case:   O(n + range)
          where range = range of input (max - min + 1)

    Space Complexity: O(n + range)

    Stability: Stable
*/

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

  int range = max - min + 1;

  int *count = (int *)calloc(range, sizeof(int));
  for (int i = 0; i < n; i++)
    count[arr[i] - min]++;

  for (int i = 1; i < range; i++)
    count[i] += count[i - 1];

  int *output = (int *)malloc(n * sizeof(int));
  for (int i = n - 1; i >= 0; i--)
    output[--count[arr[i] - min]] = arr[i];

  memcpy(arr, output, n * sizeof(int));

  free(count);
  free(output);

  printf("Counting Sort Result: ");
  for (int i = 0; i < n; i++)
    printf("%d ", arr[i]);
  printf("\n");

  return 0;
}