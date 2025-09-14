#include <stdio.h>

int main() {
    int N;
    scanf("%d", &N);

    int age, min, max;
    scanf("%d", &age);
    min = max = age;

    for (int i = 1; i < N; i++) {
        scanf("%d", &age);
        if (age < min) min = age;
        if (age > max) max = age;
    }

    if (max - min <= 3) {
        printf("homogeneous\n");
    } else {
        printf("heterogeneous\n");
    }

    return 0;
}

// 5
// 15 16 14 13 14

// 4
// 12 18 15 20
