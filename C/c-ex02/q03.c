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
    unsigned int num;
    scanf("%u", &num);
    printf("%lu\n", reverse(num));
    return 0;
}
