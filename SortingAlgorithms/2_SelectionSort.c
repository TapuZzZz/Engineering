#include <stdio.h>

void swap(int* a, int* b);

int main(){
    int arr[] = {9, 8, 7, 6, 5, 4, 3, 2, 1};
    int n = sizeof(arr) / sizeof(arr[0]);
    int MinOffset = 0;

    for(int i = 0; i < n-1; i++){
        MinOffset = i;
        for(int j = i+1; j < n; j++){
            if (arr[j] < arr[MinOffset]){
                MinOffset = j;
            }
        }
        swap(&arr[i], &arr[MinOffset]);
    }

    printf("Sorted array: ");
    for(int i = 0; i < n; i++){
        printf("%d  ",arr[i]);
    }
    printf("\n");

    return 0;
}

void swap(int* a, int* b){
    int temp = *a;
    *a = *b;
    *b = temp;
}