#include <stdio.h>
#include <stdlib.h>

typedef struct StackNode {
    int data;
    StackNode* next;
} StackNode;

void iniStack(StackNode** stack) {
    *stack = NULL;
}

unsigned short isEmpty(StackNode* stack) {
    return stack == NULL;
}

void push(StackNode** stack, int data) {
    StackNode* newNode = (StackNode*)malloc(sizeof(StackNode));

    newNode->data = data;
    newNode->next = *stack;
    *stack = newNode;
}

int pop(StackNode** stack) {
    StackNode* temp = *stack;
    int poppedValue = temp->data;
    
    *stack = (*stack)->next;
    free(temp);

    return poppedValue;
}

int main(void) {
    StackNode* myStack;
    iniStack(&myStack);

    // יצירת 4 חוליות
    push(&myStack, 10);
    push(&myStack, 20);
    push(&myStack, 30);
    push(&myStack, 40);

    // ביצוע POP פעמיים והדפסת הערכים
    printf("Popped value 1: %d\n", pop(&myStack));
    printf("Popped value 2: %d\n", pop(&myStack));

    return 0;
}