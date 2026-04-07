#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *bankMainFile;

void printManu();

int main() {
    bankMainFile = fopen("Account's_db.bin", "ab+");

    if (bankMainFile == NULL) {
        perror("Error opening file");
        return 1;
    }

    char choice = '0';

    do {
        printManu();

        choice = fgetc(stdin);
        while (fgetc(stdin) != '\n');

    } while (choice < '3');

    fclose(bankMainFile);

    return 0;
}

void printManu() {
    fprintf(stdout, "\n\n--- Hello , Welcome to the Bank\n");
    fprintf(stdout, "--- Please select the wanted operation \n");
    fprintf(stdout, "--- 1) Create new Bank account \n");
    fprintf(stdout, "--- 2) Trunsfer Funds \n");
    fprintf(stdout, "--- 3) Logout \n");
}