#include <stdio.h>
#include <stdlib.h>

typedef struct StackNode {
  void *value;
  struct StackNode *next;
} StackNode;

void Init_Stack(StackNode **stack) {
  *stack = NULL;
}

int IsEmpty_Stack(StackNode *stack) {
  return (stack == NULL);
}

void Push_Stack(StackNode **stack, void *data) {
  StackNode *newStackNode = (StackNode*)malloc(sizeof(StackNode));
  newStackNode->value = data;
  newStackNode->next = *stack;
  *stack = newStackNode;
}

void *Pop_Stack(StackNode **stack) {
  StackNode *StackNodeToPop = *stack;
  void *poppedValue = StackNodeToPop->value;
  *stack = (*stack)->next;
  free(StackNodeToPop);
  return poppedValue;
}