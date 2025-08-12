// 6. Define an array of 5 real (float) elements and assign to it the sum of each pair of corresponding elements from the arrays in the previous question.
#include <stdio.h>

int main() {
    float arr[5];
    float arrSecond[5];
    float arrThird[5];

    printf("First arr: \n");
    for (int i = 0; i < 5; i++){
        printf("EnterNum in First arr: ");
        scanf("%f", &arr[i]);
    }

    printf("Second arr: \n");
    for (int i = 0; i < 5; i++){
        printf("EnterNum in Second arr: ");
        scanf("%f", &arrSecond[i]);
    }

    for (int i = 0; i < 5; i++){
        float temp = arr[i];
        arr[i] = arrSecond[i];
        arrSecond[i] = temp;
    }

    printf("First array after swap:\n");
    for (int i = 0; i < 5; i++) {
        printf("%.2f ", arr[i]);
    }
    printf("\n");

    printf("Second array after swap:\n");
    for (int i = 0; i < 5; i++) {
        printf("%.2f ", arrSecond[i]);
    }
    printf("\n");

    for (int i = 0; i < 5; i++){
        arrThird[i] = arr[i] + arrSecond[i];
    }
    printf("Third array after sum:\n");
    for (int i = 0; i < 5; i++) {
        printf("%.2f ", arrThird[i]);
    }
    printf("\n");

    return 0;
}