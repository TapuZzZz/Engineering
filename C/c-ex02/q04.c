#include <stdio.h>

unsigned long reverse(unsigned int x) {
    unsigned long rev = 0;
    while (x > 0) {
        rev = rev * 10 + (x % 10);
        x /= 10;
    }
    return rev;
}

unsigned long merge(unsigned int a, unsigned int b) {
    unsigned long revA = reverse(a);
    unsigned long revB = reverse(b);
    unsigned long result = 0;

    while (revA > 0 || revB > 0) {
        if (revA > 0) {
            result = result * 10 + (revA % 10);
            revA /= 10;
        }
        if (revB > 0) {
            result = result * 10 + (revB % 10);
            revB /= 10;
        }
    }
    return result;
}

int main() {
    unsigned int a, b;
    scanf("%u %u", &a, &b);
    printf("%lu\n", merge(a, b));
    return 0;
}
