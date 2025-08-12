// A magic square is a square in which the sum of the elements of each row is equal to the sum of the elements of each column and the sum of each diagonal.
// Take an array of type Integer of size 3*3 and print out whether it is a magic square.

#include <stdio.h>
#define SIZE 3

int mat[SIZE][SIZE];

void inputMatrix() {
    printf("Enter the elements of %dx%d matrix:\n", SIZE, SIZE);
    for(int i = 0; i < SIZE; i++) {
        for(int j = 0; j < SIZE; j++) {
            printf("Enter element [%d][%d]: ", i+1, j+1);
            scanf("%d", &mat[i][j]);
        }
    }
}

void printMatrix() {
    printf("\nYour matrix:\n");
    for(int i = 0; i < SIZE; i++) {
        for(int j = 0; j < SIZE; j++) {
            printf("%d\t", mat[i][j]);
        }
        printf("\n");
    }
}

int isMagicSquare() {
    int magicSum = 0, sum;
    
    // Calculate magic sum using first row
    for(int j = 0; j < SIZE; j++) {
        magicSum += mat[0][j];
    }
    
    // Check all rows
    for(int i = 0; i < SIZE; i++) {
        sum = 0;
        for(int j = 0; j < SIZE; j++) {
            sum += mat[i][j];
        }
        if(sum != magicSum) {
            return 0;
        }
    }
    
    // Check all columns
    for(int j = 0; j < SIZE; j++) {
        sum = 0;
        for(int i = 0; i < SIZE; i++) {
            sum += mat[i][j];
        }
        if(sum != magicSum) {
            return 0;
        }
    }
    
    // Check main diagonal
    sum = 0;
    for(int i = 0; i < SIZE; i++) {
        sum += mat[i][i];
    }
    if(sum != magicSum) {
        return 0;
    }
    
    // Check anti-diagonal
    sum = 0;
    for(int i = 0; i < SIZE; i++) {
        sum += mat[i][SIZE - 1 - i];
    }
    if(sum != magicSum) {
        return 0;
    }
    
    return 1;
}

int main(){
    inputMatrix();
    printMatrix();
    
    if(isMagicSquare()) {
        printf("\nYES! This is a magic square.\n");
    } else {
        printf("\nNO! This is not a magic square.\n");
    }
    
    return 0;
}