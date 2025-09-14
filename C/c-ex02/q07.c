#include <stdio.h>
#define PI 3.141592653589793

int main() {
    double r1, r2, area1, area2, averageCircumference, thickness;

    scanf("%lf %lf", &r1, &r2);

    if (r1 > r2) {
        double temp = r1;
        r1 = r2;
        r2 = temp;
    }

    // א. שטח הטבעת = שטח המעגל הגדול - שטח המעגל הקטן
    area1 = PI * r2 * r2 - PI * r1 * r1;

    // ב. שטח הטבעת = ממוצע היקפים * עובי הטבעת
    thickness = r2 - r1;
    averageCircumference = PI * 2 * (r1 + r2) / 2;
    area2 = averageCircumference * thickness;

    printf("%.6f\n", area1);
    printf("%.6f\n", area2);

    return 0;
}
