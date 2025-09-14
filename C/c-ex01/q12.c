#include <stdio.h>

int main() {
    int a;
    scanf("%d", &a);

    for (int tens = 1; tens <= 9; tens++) {
        int units = a - tens; 
        if (units >= 0 && units <= 9) {
            printf("%d ", tens * 10 + units);
        }
    }
    printf("\n");

    return 0;
}
