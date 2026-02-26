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
  StackNode *newNode = (StackNode *)malloc(sizeof(StackNode));
  newNode->value = data;

  newNode->next = *stack;
  *stack = newNode;
}

void *Pop_Stack(StackNode **stack) {
  StackNode *temp = *stack;
  void *poppedValue = temp->value;

  *stack = (*stack)->next;
  free(temp);

  return poppedValue;
}