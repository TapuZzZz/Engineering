#include <stdio.h>
/*
    Shell Sort Analysis:
    ────────────────────────────────────────────────────────────────────
    How it works:
        1. Instead of comparing adjacent elements (like Insertion Sort),
           compare elements that are 'gap' positions apart.
        2. Start with a large gap (n/2) and shrink it by half each pass
           until gap = 1 (a final plain Insertion Sort pass).
        3. Each pass partially sorts the array, so the final pass is
           much cheaper than a naive Insertion Sort from scratch.

    Time Complexity:
        - Best Case:    O(n log n)   (nearly sorted input)
        - Average Case: O(n log² n)  (depends on gap sequence)
        - Worst Case:   O(n²)        (with Shell's original gap sequence)

    Space Complexity: O(1)

    Stability: Unstable
*/

int main() {
  int arr[] = {9, 8, 7, 6, 5, 4, 3, 2, 1};
  int n = sizeof(arr) / sizeof(arr[0]);

  for (int gap = n / 2; gap > 0; gap /= 2) {
    for (int i = gap; i < n; i++) {
      int temp = arr[i];
      int j = i;
      while (j >= gap && arr[j - gap] > temp) {
        arr[j] = arr[j - gap];
        j -= gap;
      }
      arr[j] = temp;
    }
  }

  printf("Shell Sort Result: ");
  for (int i = 0; i < n; i++)
    printf("%d ", arr[i]);
  printf("\n");

  return 0;
}