#include <stdio.h>

int main() {
    // 1. הגדרת משתנים
    FILE *fptr;
    char name[50] = "Shurna";
    int age = 25;
    char buffer[100]; // משתנה שיאחסן טקסט שנקרא מהקובץ

    // --- שלב הכתיבה (WRITE) ---

    // פתיחת קובץ לכתיבה - אם הוא לא קיים, הוא ייווצר
    fptr = fopen("learning.txt", "w");

    if (fptr == NULL) {
        printf("Error: Could not create file.\n");
        return 1;
    }

    // כתיבת טקסט ומשתנים לקובץ
    fprintf(fptr, "Name: %s\n", name);
    fprintf(fptr, "Age: %d\n", age);
    
    // סגירת הקובץ כדי לשמור את השינויים
    fclose(fptr);
    printf("Data written to file successfully.\n");

    // --- שלב הקריאה (READ) ---

    // פתיחת אותו קובץ, הפעם למטרת קריאה בלבד (r)
    fptr = fopen("learning.txt", "r");

    if (fptr == NULL) {
        printf("Error: Could not open file for reading.\n");
        return 1;
    }

    printf("\nReading from file:\n");

    // שימוש ב-fgets כדי לקרוא שורה אחר שורה עד סוף הקובץ
    // fgets מחזירה NULL כשהיא מגיעה לסוף הקובץ
    while (fgets(buffer, 100, fptr) != NULL) {
        printf("%s", buffer); // מדפיס למסך את מה שנקרא מהקובץ
    }

    // סגירה סופית
    fclose(fptr);

    return 0;
}