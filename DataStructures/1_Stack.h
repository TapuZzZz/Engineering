#ifndef Stack_H
#define Stack_H

typedef struct StackNode {
    int data;
    StackNode* next;
} StackNode;


void iniStack(StackNode** stack);
unsigned short isEmpty(StackNode* stack);
void push(StackNode** stack, int data);
int pop(StackNode** stack);

#endif