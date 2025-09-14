#include <stdio.h>

int main() {
    printf("%6d\n", 1);
    printf("%6d\n", 11);
    printf("%6d\n", 111);
    printf("%6d\n", 1111);
    printf("%6d\n", 11111);
    printf("%6d\n", 111111);

    printf("%4d\n", 12345);

    printf("%-6d\n", 123);

    printf("%8.2f\n", 3.345);
    printf("%-8.2f\n", 3.345);
    printf("%08.2f\n", 3.345);

    return 0;
}
