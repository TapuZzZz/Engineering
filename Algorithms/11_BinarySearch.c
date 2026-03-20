#include <stdio.h>
/*
    Binary Search Analysis:
    ────────────────────────────────────────────────────────────────────
    How it works:
        1. Requires a sorted array.
        2. Compare the target with the middle element of the current range.
        3. If equal → found. If target is smaller → search left half.
           If target is larger → search right half.
        4. Repeat (iterative) or recurse until found or range is empty.

    Time Complexity:
        - Best Case:    O(1)        (target is the middle element)
        - Average Case: O(log n)
        - Worst Case:   O(log n)

    Space Complexity:
        - Iterative: O(1)
        - Recursive: O(log n)  (call stack)
*/

int binarySearchIterative(int *arr, int n, int target) {
  int low = 0, high = n - 1;
  while (low <= high) {
    int mid = low + (high - low) / 2;
    if (arr[mid] == target)
      return mid;
    if (arr[mid] < target)
      low = mid + 1;
    else
      high = mid - 1;
  }
  return -1;
}

int binarySearchRecursive(int *arr, int low, int high, int target) {
  if (low > high)
    return -1;
  int mid = low + (high - low) / 2;
  if (arr[mid] == target)
    return mid;
  if (arr[mid] < target)
    return binarySearchRecursive(arr, mid + 1, high, target);
  else
    return binarySearchRecursive(arr, low, mid - 1, target);
}

int main() {
  int arr[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  int n = sizeof(arr) / sizeof(arr[0]);
  int target = 6;

  int idxI = binarySearchIterative(arr, n, target);
  int idxR = binarySearchRecursive(arr, 0, n - 1, target);

  printf("Iterative: target %d found at index %d\n", target, idxI);
  printf("Recursive: target %d found at index %d\n", target, idxR);

  return 0;
}