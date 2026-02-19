#include <stdio.h>
/*
    Selection Sort Analysis:
    
    How it works:
        1. Scan the unsorted part of the array to find the index of the minimum element.
        2. Once the minimum is found, swap it with the first element of the unsorted part.
        3. This reduces the number of swaps to exactly one per outer loop iteration.
    
    Time Complexity:
        - Best Case: O(n²)
        - Average Case: O(n²)
        - Worst Case: O(n²)
        
    Stability: Unstable
        Because it picks the minimum and swaps it over long distances, it can 
        jump over identical elements and change their original order.
*/
void swap(int* a, int* b);

int main() {
    int arr[] = {9, 8, 7, 6, 5, 4, 3, 2, 1};
    int n = sizeof(arr) / sizeof(arr[0]);

    for (int i = 0; i < n - 1; i++) {
        int offset = i;
        for (int j = i + 1; j < n; j++) {
            if (arr[j] < arr[offset]) {
                offset = j;
            }
        }
        if (offset != i) {
            swap(&arr[i], &arr[offset]);
        }
    }

    printf("Selection Sort Result: ");
    for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");

    return 0;
}

void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}