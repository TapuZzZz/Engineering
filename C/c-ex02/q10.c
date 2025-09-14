#include <stdio.h>

unsigned long reverse(unsigned int x) {
    unsigned long rev = 0;
    while (x > 0) {
        rev = rev * 10 + (x % 10);
        x /= 10;
    }
    return rev;
}

int main() {
    for (unsigned int num = 10000; num <= 99999; num++) {
        if (4 * num == reverse(num)) {
            printf("%u\n", num);
        }
    }
    return 0;
}
