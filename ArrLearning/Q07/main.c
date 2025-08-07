// Define an array of 8 elements to receive the salaries of 8 employees and print the highest and lowest salary.
// If the highest/lowest number appears more than once - indicate that.
#include <stdio.h>
int main() {
    int arrSalary[] = {110,250,320,410,335,410,240,160};
    int n = sizeof(arrSalary) / sizeof(arrSalary[0]);
    int maxSalary = arrSalary[0];
    int minSalary = arrSalary[0];
    int countMax = 0;
    int countMin = 0;

    // Find max and min
    for (int i = 1; i < n; i++) {
        if (arrSalary[i] > maxSalary)
            maxSalary = arrSalary[i];
        if (arrSalary[i] < minSalary)
            minSalary = arrSalary[i];
    }

    // Count occurrences
    for (int i = 0; i < n; i++) {
        if (arrSalary[i] == maxSalary)
            countMax++;
        if (arrSalary[i] == minSalary)
            countMin++;
    }

    printf("The highest salary is: %d\n", maxSalary);
    if (countMax > 1)
        printf("The highest salary appears %d times.\n", countMax);

    printf("The lowest salary is: %d\n", minSalary);
    if (countMin > 1)
        printf("The lowest salary appears %d times.\n", countMin);

    return 0;
}