// 11. Define an array of 5 values of type short.
// Get values from the user into the array and print all the digits of the array.
#include <stdio.h>

short arr[5];

int main(){

    for (int i = 0; i < 5; i++){
        printf("Enter num %d : ",(i+1));
        scanf("%hd",&arr[i]);
    }   

    printf("===========\n");

    for (int i = 0; i < 5; i++){
        printf("num %d is : %hd\n",(i+1),arr[i]);
    }   

    return 0;

}
