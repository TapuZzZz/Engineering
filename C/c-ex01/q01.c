#include <stdio.h>  
#define TOTOSIZE 15  

int main()  
{  
    int d = 0, i;  
    char score;  
    
    for (i=1; i<=TOTOSIZE; i++) {  
        scanf ("%c", &score);  
        if (score == 'X')  
            d++;  
    }  
    printf ("%d", d);  
    return 0;  
}  

// A. 5
// B. 0 → 111111111111111, 15 → XXXXXXXXXXXXXXX
// C. there is only one 'X' in the input
// D. 0 - 15
// E. counts how meny games over with draw
// F. drawCount