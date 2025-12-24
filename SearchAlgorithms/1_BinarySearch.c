#include <stdio.h>

int binarySearchIter(const int arr[], int n, int x) {
    int low = 0, high = n - 1;

    while (low <= high) {
        int mid = low + ((high - low) >> 1);

        if (arr[mid] == x)
            return mid;

        if (arr[mid] < x)
            low = mid + 1;
        else
            high = mid - 1;
    }

    return -1;
}

int binarySearchRec(const int arr[], int low, int high, int x) {
    if (low > high)
        return -1;

    int mid = low + ((high - low) >> 1);

    if (arr[mid] == x)
        return mid;

    if (arr[mid] < x)
        return binarySearchRec(arr, mid + 1, high, x);

    return binarySearchRec(arr, low, mid - 1, x);
}

int main(void) {

    return 0;
}
