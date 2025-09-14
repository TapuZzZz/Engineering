#include <stdio.h>

int main() {
    int n;
    scanf("%d", &n);

    int evenCount = 0;
    int oddCount = 0;
    int temp = n;

    while (temp > 0) {
        int digit = temp % 10;
        if (digit % 2 == 0)
            evenCount++;
        else
            oddCount++;
        temp /= 10;
    }

    if (evenCount > oddCount)
        printf("גדול\n");
    else if (evenCount < oddCount)
        printf("קטן\n");
    else
        printf("שווה\n");

    return 0;
}
