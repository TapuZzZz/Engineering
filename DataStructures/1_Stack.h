#ifndef STACK.H
#define STACK.H

typedef struct StackNode {
  void *value;
  struct StackNode *next;
} StackNode;

void Init_Stack(StackNode **stack);

int IsEmpty_Stack(StackNode *stack);

void Push_Stack(StackNode **stack, void *data);

void *Pop_Stack(StackNode **stack);

#endif