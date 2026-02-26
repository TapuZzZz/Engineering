#include <stdio.h>
#include <stdlib.h>

typedef struct LLLNode {
    void *value;
    struct LLLNode *next;
} LLLNode;

void Init_LLL(LLLNode **head) { 
    *head = NULL; 
}

int IsEmpty_LLL(LLLNode *head) {
    return (head == NULL);
}

void Push_LLL(LLLNode **head, void *data) {
    LLLNode *newLLLNode = (LLLNode*)malloc(sizeof(LLLNode));
    newLLLNode->value = data;
    newLLLNode->next = NULL;
    *head = newLLLNode;
}

void InsertAfter_LLL(LLLNode *prevLLLNode, void *data) {
    LLLNode *newLLLNode = (LLLNode*)malloc(sizeof(LLLNode));
    newLLLNode->value = data;
    newLLLNode->next = prevLLLNode->next;
    prevLLLNode->next = newLLLNode;
}

void* Pop_LLL(LLLNode **head) {
    LLLNode *LLLnodeToDelete = *head;
    void* poppedValue = LLLnodeToDelete->value;
    
    *head = NULL;
    free(LLLnodeToDelete);

    return poppedValue;
}

void* DeleteAfter_LLL(LLLNode *prevLLLNode) {
    LLLNode *LLLnodeToDelete = prevLLLNode->next;
    void* deletedValue = LLLnodeToDelete->value;
    
    prevLLLNode->next = LLLnodeToDelete->next;
    free(LLLnodeToDelete);

    return deletedValue;
}