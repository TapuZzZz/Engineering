// 9. Write a program that receives grades for 25 students and prints:
// a. The average grade
// b. How many students failed (below 55)
// c. The highest grade and how many students received it
#include <stdio.h>

int main() {

    float average = 0;
    int numFail = 0;
    int highestGrade = 0;
    int cntHighestGrade = 0;
    int grade = 0;
    int n = 25;

    for (int i = 0; i < n; i++){
        printf("Enter grade: ");
        scanf("%d", &grade);
        printf("\n");

        average += grade;

        if (grade < 55)
            numFail++;
        
        if (grade > highestGrade){
            cntHighestGrade = 1;
            highestGrade = grade;
        } 
        else if (grade == highestGrade) {
            cntHighestGrade++;
        }
    }

    printf("Average = %.2f \n", (average / n));
    printf("Number of failed students: %d\n", numFail);
    printf("Highest grade: %d\n", highestGrade);
    printf("Number of students with highest grade: %d\n", cntHighestGrade);

    return 0;
}