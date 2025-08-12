// 1. Write a program that defines an array of 5 integers and inputs data into it
#include <stdio.h>

int main() {
    int arr[5];
    
    printf("Enter 5 integers:\n");

    for (int i = 0; i < 5; i++) {
        printf("Element %d: ", i + 1);
        scanf("%d", &arr[i]);
    }

    for (int i = 0; i < 5; i++) {
        printf("%d ", arr[i]);
    }

    printf("\n");

    return 0;
}