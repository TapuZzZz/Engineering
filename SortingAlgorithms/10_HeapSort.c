#include <stdio.h>
/*
    Heap Sort Analysis:
    ────────────────────────────────────────────────────────────────────
    How it works:
        1. Build a max-heap from the array in-place (heapify all
           non-leaf nodes from bottom up).
        2. The root is now the largest element — swap it with the last
           element, then reduce the heap size by 1.
        3. Heapify the root again to restore the max-heap property.
        4. Repeat steps 2-3 until the heap size reaches 1.

    Time Complexity:
        - Best Case:    O(n log n)
        - Average Case: O(n log n)
        - Worst Case:   O(n log n)

    Space Complexity: O(1)

    Stability: Unstable
*/

static void swap(int *a, int *b) {
  int temp = *a;
  *a = *b;
  *b = temp;
}

static void heapify(int *arr, int n, int i) {
  int largest = i;
  int left = 2 * i + 1;
  int right = 2 * i + 2;

  if (left < n && arr[left] > arr[largest])
    largest = left;
  if (right < n && arr[right] > arr[largest])
    largest = right;

  if (largest != i) {
    swap(&arr[i], &arr[largest]);
    heapify(arr, n, largest);
  }
}

int main() {
  int arr[] = {9, 3, 7, 1, 8, 2, 6, 4, 5};
  int n = sizeof(arr) / sizeof(arr[0]);

  for (int i = n / 2 - 1; i >= 0; i--)
    heapify(arr, n, i);

  for (int i = n - 1; i > 0; i--) {
    swap(&arr[0], &arr[i]);
    heapify(arr, i, 0);
  }

  printf("Heap Sort Result: ");
  for (int i = 0; i < n; i++)
    printf("%d ", arr[i]);
  printf("\n");

  return 0;
}