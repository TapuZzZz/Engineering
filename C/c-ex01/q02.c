// A. (6, 3) → 18
//    (3, -2) → 3
//    (5, 4) → 9
//    (4, 5) → 9

// B.
#include <stdio.h>

int main() {
    int a, b;
    int sum = 0;
    
    scanf("%d %d", &a, &b);

    int start = (a < b) ? a : b;
    int end = (a > b) ? a : b;

    for (int i = start; i <= end; i++) {
        sum += i;
    }

    printf("%d\n", sum);
    return 0;
}

// C. Done
