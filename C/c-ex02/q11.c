#include <stdio.h>

int main() {
    int digits[9], i, sum = 0;
    
    for (i = 0; i < 9; i++) {
        scanf("%1d", &digits[i]);
    }

    for (i = 0; i < 8; i++) {
        int val = digits[i];
        if (i % 2 == 0) val *= 1;      
        else val *= 2;

        if (val > 9) val -= 9;     
        sum += val;
    }

    int checkDigit = (10 - (sum % 10)) % 10;

    if (checkDigit == digits[8])
        printf("תקין\n");
    else
        printf("לא תקין\n");

    return 0;
}
