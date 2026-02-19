#include <stdio.h>
/*
    Exchange Sort Analysis:
    
    How it works:
        1. Compare the current element at index (i) with every element that follows it (j).
        2. If any following element is smaller than the current element, swap them immediately.
        3. This "exchanges" the values until the smallest value for the current position is found.
    
    Time Complexity:
        - Best Case: O(n²)
        - Average Case: O(n²)
        - Worst Case: O(n²)

    Space Complexity: O(1)
        
    Stability: Unstable
        It performs swaps across multiple positions, which can change the relative 
        order of equal elements.
*/
void swap(int* a, int* b);

int main() {
    int arr[] = {9, 8, 7, 6, 5, 4, 3, 2, 1};
    int n = sizeof(arr) / sizeof(arr[0]);

    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (arr[j] < arr[i]) {
                swap(&arr[i], &arr[j]);
            }
        }
    }

    printf("Exchange Sort Result: ");
    for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");

    return 0;
}

void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}