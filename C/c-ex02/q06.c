#include <stdio.h>

int main() {
    int num;
    for (num = 1000; num <= 9999; num++) {
        int left = num / 100;
        int right = num % 100;
        int sum = left + right;
        if (sum * sum == num) {
            printf("%d\n", num);
        }
    }
    return 0;
}
