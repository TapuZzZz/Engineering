#include <stdio.h>
#include <stdlib.h>

typedef struct Stack {
    int data;
    struct Stack* next;
} Stack;

void iniStack(Stack** stack) {
    *stack = NULL;
}

unsigned short isEmpty(Stack* stack) {
    return stack == NULL;
}

void push(Stack** stack, int data) {
    Stack* newNode = (Stack*)malloc(sizeof(Stack));

    newNode->data = data;
    newNode->next = *stack;
    *stack = newNode;
}

int pop(Stack** stack) {
    Stack* temp = *stack;
    int poppedValue = temp->data;
    
    *stack = (*stack)->next;
    free(temp);

    return poppedValue;
}

int main(void) {
    
    return 0;
}