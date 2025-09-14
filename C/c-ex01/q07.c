#include <stdio.h>

int main() {
    int seconds_total;
    scanf("%d", &seconds_total);

    int hours = seconds_total / 3600;
    int minutes = (seconds_total % 3600) / 60;
    int seconds = seconds_total % 60;

    printf("%02d:%02d:%02d\n", hours, minutes, seconds);

    return 0;
}
