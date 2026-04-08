#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

typedef enum {
    sunday,
    monday,
    tuesday,
    wednesday,
    thursday,
    friday,
    saturday
} daysOfTheWeek;

void myLittleFunc(int *numToSave, int num) {
    *numToSave = num;
}

int main() {

    daysOfTheWeek today = thursday;
    printf("%d\n", sunday);
    printf("%d\n\n", thursday);


    int num1 = 5;
    printf("%f\n", pow(num1,2));

    float myNum = -4.5;
    printf("%f\n", round(myNum));
    printf("%f\n", ceil(myNum));
    printf("%f\n", floor(myNum));
    printf("%f\n", trunc(myNum));
    printf("%d\n\n", abs(myNum));


    srand(time(NULL));
    int r = rand() % 10;
    printf("%d\n\n", r);


    int num = 0;
    //scanf("%x", &num);

    printf("%x\n", num);
    printf("%d\n", num);

    int condition = 14;
    printf("%d\n", !condition);
    printf("%d\n", condition && 4);
    printf("%d\n\n", condition || 2);

    int choice = 0;
    scanf("%d", &choice);
    switch (choice) {
        case 1: 
            printf("choice is 1!");
            break;
        case 2: 
            printf("choice is 2!");
            break;
        case 3: 
            printf("choice is 3!");
            break;
        default:
            printf("Error");
            break;
    }

    int xNum = 261;
    int *p_xNum = &xNum;
    int **pp_xNum = &p_xNum;

    *(pp_xNum) = &choice;

    printf("\n\n%d\n\n", *p_xNum);

    int number = 5;
    myLittleFunc(&number, 7);
    printf("%d\n\n", number);

    


    return 0;
}