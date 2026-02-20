#include <stdio.h>
/*
    Insertion Sort Analysis:
    ────────────────────────────────────────────────────────────────────
    How it works:
        1. Start from the second element (offset 1).
        2. Pick the element (key) and compare it with the elements before it.
        3. Shift all elements larger than the key to the right.
        4. Insert the key into its correct position.
    
    Time Complexity:
        - Best Case: O(n) -> (When the array is already sorted)
        - Average Case: O(n²)
        - Worst Case: O(n²)

    Space Complexity: O(1)
        
    Stability: Stable 
*/

int main() {
    int arr[] = {9, 8, 7, 6, 5, 4, 3, 2, 1};
    int n = sizeof(arr) / sizeof(arr[0]);

    for(int i = 1; i < n ; i++) {
        int temp = arr[i];
        int j = i - 1;

        while(arr[j] > temp && j >= 0){
            arr[j+1] = arr[j]; 
            j--;
        }
        arr[j+1] = temp;
    }

    printf("Selection Sort Result: ");
    for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");

    return 0;
}