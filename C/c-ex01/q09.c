#include <stdio.h>
#define A 'c'
#define B 'b'
#define C 'd'
#define D 'a'

int main()
{
    char a = C, b = 'd', c = D, d = a;
    printf ("%c %c %c %c \n", 'd' , D , b , a);
    return 0;
}

// answer -> (3):  b , 'c' , B , c

