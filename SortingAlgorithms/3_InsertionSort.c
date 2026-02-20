#include <stdio.h>
#include <stdlib.h>
/*
    Bucket Sort Analysis:
    ────────────────────────────────────────────────────────────────────
    How it works:
        1. Create n empty buckets.
        2. Distribute each element into a bucket based on its value range.
        3. Sort each individual bucket using insertion sort.
        4. Concatenate all buckets back into the original array.

    Time Complexity:
        - Best Case:    O(n)   -> (When input is uniformly distributed)
        - Average Case: O(n)
        - Worst Case:   O(n²)  -> (When all elements land in one bucket)

    Space Complexity: O(n)
    
    Stability: Stable
*/
int main() {
    float arr[] = {0.78f, 0.17f, 0.39f, 0.26f, 0.72f, 0.94f, 0.21f, 0.12f, 0.68f, 0.55f};
    int n = sizeof(arr) / sizeof(arr[0]);

    // Step 1: Create n empty buckets
    float **buckets   = (float **)calloc(n, sizeof(float *));
    int   *bucketSize = (int *)calloc(n, sizeof(int));
    for (int i = 0; i < n; i++)
        buckets[i] = (float *)malloc(n * sizeof(float));

    // Step 2: Distribute elements into buckets
    for (int i = 0; i < n; i++) {
        int idx = (int)(arr[i] * n);
        if (idx >= n) idx = n - 1;
        buckets[idx][bucketSize[idx]++] = arr[i];
    }

    // Step 3: Sort each bucket using insertion sort
    for (int i = 0; i < n; i++) {
        for (int j = 1; j < bucketSize[i]; j++) {
            float temp = buckets[i][j];
            int k = j - 1;
            while (k >= 0 && buckets[i][k] > temp) {
                buckets[i][k + 1] = buckets[i][k];
                k--;
            }
            buckets[i][k + 1] = temp;
        }
    }

    // Step 4: Concatenate all buckets back into arr
    int pos = 0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < bucketSize[i]; j++)
            arr[pos++] = buckets[i][j];

    // Cleanup
    for (int i = 0; i < n; i++) free(buckets[i]);
    free(buckets);
    free(bucketSize);

    printf("Bucket Sort Result: ");
    for (int i = 0; i < n; i++) printf("%.2f ", arr[i]);
    printf("\n");
    return 0;
}