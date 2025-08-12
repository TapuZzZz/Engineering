// 4. Declare another integer array of 10 elements and copy into it the array from question 3
#include <stdio.h>

int main() {
    int arr[10];
    int arrCopy[10];
    
    for (int i = 0; i < 10; i++){
        arr[i] = 10 - i;
    }

    for (int i = 0; i < 10; i++){
        arrCopy[i] = arr[i];
    }

    for (int i = 0; i < 10; i++){
        printf("%d ", arrCopy[i]); 
    }

    printf("\n");
    
    return 0;
}