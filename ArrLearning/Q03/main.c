// Define an array of 10 integers and, using a loop, insert into it the numbers from 10 down to 1 in descending order.
#include <stdio.h>

int main() {
    int arr[10];
    
    for (int i = 0; i < 10; i++){
        arr[i] = 10 - i;
    }

    for (int i = 0; i < 10; i++){
        printf("%d ", arr[i]); 
    }

    printf("\n");
    
    return 0;
}