// 13. Define 2 two-dimensional arrays of integers of size 4*4. 
// Enter numbers into one of them. Write a program that will fill the second array while reversing the rows and columns. 
// That is, the first row will become the first column and vice versa, and so on.
#include <stdio.h>

int mat[4][4];
int mat2[4][4];

int main(){

    for(int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){

            printf("Enter num: ");
            scanf("%d",&mat[i][j]);
        
        }
    }

    printf("========================\n");

    for(int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
            
            printf("%5d ", mat[i][j]);

        }
        printf("\n");
    }

    printf("************************\n");

    for(int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){

            mat2[j][i] = mat[i][j];
        
        }
    }


    for(int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
            
            printf("%5d ", mat2[i][j]);

        }
        printf("\n");
    }


    return 0;
}
