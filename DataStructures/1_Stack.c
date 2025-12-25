#include <stdio.h>
#include <stdlib.h>

typedef int sType;

typedef struct{
    struct Stack* nextItem;
    sType value;
} Stack;

Stack* iniStack() {
    Stack* s = NULL;
    return s;
}

unsigned short isEmpty(Stack *s) {
    return s == NULL;
}

unsigned short pop(Stack **s) {
    sType headVal = (*s)->value;
    *s = (*s)->nextItem;
    return headVal;
}

void push(Stack **s,  sType  val) {
    if (!isEmpty(*s)) {
        Stack* NI = (Stack*)malloc(sizeof(Stack));
        NI->value = val;
        NI->nextItem = *s;
        *s = NI;
    } else {
        Stack* firstItem = (Stack*)malloc(sizeof(Stack));
        firstItem->value = val;
        firstItem->nextItem = NULL;
        *s = firstItem;
    }
}

int main(void){
    return 0;// Im Home
}