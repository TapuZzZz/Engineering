// 2. Define an array of 10 integers and use a loop to insert numbers from 1 to 10 in ascending order.
#include <stdio.h>

int main() {
    int arr[10];
    
    for (int i = 0; i < 10; i++){
        arr[i] = i + 1;
    }

    for (int i = 0; i < 10; i++){
        printf("%d ", arr[i]); 
    }

    printf("\n");
    
    return 0;
}