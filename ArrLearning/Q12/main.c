// 12. Define a two-dimensional array of numbers of size 4*4. Enter numbers into it. Calculate and print: 
// a. The sum of each row 
// b. The sum of each column 
// c. The sum of each diagonal.
#include <stdio.h>

int mat[4][4];

int main(){

    for(int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){

            printf("Enter num: ");
            scanf("%d",&mat[i][j]);
        
        }
    }

    printf("==================\n");

    for(int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
            
            printf("%5d ", mat[i][j]);

        }
        printf("\n");
    }


    return 0;
}