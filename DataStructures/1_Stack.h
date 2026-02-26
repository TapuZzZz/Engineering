#ifndef Stack_H
#define Stack_H

typedef struct StackNode {
  void *value;
  struct StackNode *next;
} StackNode;

void iniStack(StackNode **stack);

int isEmpty(StackNode *stack);

void push(StackNode **stack, void* data);

void* pop(StackNode **stack);

#endif