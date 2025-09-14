#include <stdio.h>

int main() {
    int N, M;
    scanf("%d %d", &N, &M);

    int arr[1000];
    for (int i = 0; i < N; i++) {
        scanf("%d", &arr[i]);
    }

    int found = 0;

    for (int i = 0; i < N && !found; i++) {
        for (int j = i + 1; j < N; j++) {
            if (arr[i] + arr[j] > M) {
                found = 1;
                break;
            }
        }
    }

    if (found) {
        printf("YES\n");
    } else {
        printf("NO\n");
    }

    return 0;
}

// 5 20
// 5 10 7 3 9
// NO

// 4 10
// 8 6 4 2
// YES

