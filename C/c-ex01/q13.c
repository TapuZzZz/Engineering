#include <stdio.h>

int main() {
    for (int a = 10; a <= 99; a++) {   
        for (int b = 10; b < a; b++) {
            int b_tens = b / 10;
            int b_units = b % 10;
            int a_tens = a / 10;
            int a_units = a % 10;

            if (b_units == a_tens && a_units != 0) {
                if ((float)b / a == (float)b_tens / a_units) {
                    printf("%d/%d -> %d/%d\n", b, a, b_tens, a_units);
                }
            }
        }
    }
    return 0;
}

// 16/64 -> 1/4
// 26/65 -> 2/5
// 19/95 -> 1/5
// 49/98 -> 4/8