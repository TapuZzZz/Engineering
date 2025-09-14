#include <stdio.h>

int sumDigits(int n) {
    int sum = 0;
    while (n > 0) {
        sum += n % 10;
        n /= 10;
    }
    return sum;
}

int main() {
    int left, right;
    scanf("%d %d", &left, &right); 

    int maxSum = -1;  
    int number = right; 

    for (int i = right; i <= left; i++) {
        int s = sumDigits(i);
        if (s > maxSum) {
            maxSum = s;
            number = i;
        }
    }

    printf("%d\n", number);

    return 0;
}
