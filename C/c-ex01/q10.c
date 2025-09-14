#include <stdio.h>

int main() {
    int n, q, a1;
    scanf("%d %d %d", &n, &q, &a1);

    int sequence[n];

    sequence[0] = a1;
    for (int i = 1; i < n; i++) {
        sequence[i] = sequence[i-1] * q;
    }

    for (int i = n-1; i >= 0; i--) {
        printf("%d", sequence[i]);
        if (i > 0) printf(", ");
    }
    printf("\n");

    return 0;
}
