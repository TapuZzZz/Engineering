#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void main() {
    char *str = NULL;
    str = (char *)(malloc(sizeof(char) * 10));
    
    strcpy(str,"Artyom");

    printf("%s \n", str);
}


void emailVeryfication() {

}
