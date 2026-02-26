#include <stdio.h>
#include <stdlib.h>

typedef struct StackNode {
  void *value;
  struct StackNode *next;
} StackNode;

void iniStack(StackNode **stack) {
  *stack = NULL;
}

int isEmpty(StackNode *stack) {
  return (stack == NULL);
}

void push(StackNode **stack, void* data) {
  StackNode *newNode = (StackNode *)malloc(sizeof(StackNode));
  newNode->value = data;

  newNode->next = *stack;
  *stack = newNode;
}

void* pop(StackNode **stack) {
  StackNode *temp = *stack;
  void* poppedValue = temp->value;

  *stack = (*stack)->next;
  free(temp);

  return poppedValue;
}