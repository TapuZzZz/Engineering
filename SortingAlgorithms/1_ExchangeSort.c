#include <stdio.h>

void swap(int* a, int* b);

int main(){
    int arr[] = {5, 7, 3, 9, 2, 6};
    int n = sizeof(arr) / sizeof(arr[0]);

    for(int i = 0; i < n-1; i++){
        for(int j = i+1; j < n; j++){
            if (arr[i] > arr[j]){
                swap(&arr[i], &arr[j]);
            }
        }
    }

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