#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main() {
    // Initialize a string with a size of 100 characters
    char str[100];
    
    // Prompt the user for input
    printf("Enter a string: ");
    fgets(str, sizeof(str), stdin);
    
    // Remove the newline character if present
    str[strcspn(str, "\n")] = 0;

    // Print the entered string
    printf("You entered: %s\n", str);

    return 0;
}