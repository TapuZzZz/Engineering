#include <stdio.h>

double celsiusToFahrenheit(double c) {
    return (c * 9.0 / 5.0) + 32;
}

double fahrenheitToCelsius(double f) {
    return (f - 32) * 5.0 / 9.0;
}

int main() {
    printf("Celsius\tFahrenheit\n");
    for (int c = -273; c <= 101; c += 11) {
        double f = celsiusToFahrenheit(c);
        printf("%d\t%.2f\n", c, f);
    }
    return 0;
}
