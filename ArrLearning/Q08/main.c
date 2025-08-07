// 8. Define an array whose elements are 2, 4, 6 and an array whose elements are 3, 5, 7,
// and a third array whose elements will be the product of each pair of elements respectively.

#include <stdio.h>

int main() {
    int arr1[] = {2, 4, 6};
    int arr2[] = {3, 5, 7};
    int arrProduct[3];

    for (int i = 0; i < 3; i++){
        arrProduct[i] = arr1[i] * arr2[i];
    }

    return 0;
}