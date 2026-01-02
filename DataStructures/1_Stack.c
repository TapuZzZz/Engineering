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