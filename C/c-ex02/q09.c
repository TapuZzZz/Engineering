#include <stdio.h>
#include <math.h>

int main() {
    for (int num = 100; num <= 999; num++) {
        int hundreds = num / 100;
        int tens = (num / 10) % 10;
        int units = num % 10;
        if ((hundreds*hundreds*hundreds + tens*tens*tens + units*units*units) == num) {
            printf("%d\n", num);
        }
    }
    return 0;
}
