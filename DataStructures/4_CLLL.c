#include <stdio.h>
#include <stdlib.h>

typedef struct CLLLNode {
    void *value;
    struct CLLLNode *next;
} CLLLNode;


void Init_CLLL(CLLLNode **manager) {
    *manager = NULL;
}

int IsEmpty_CLLL(CLLLNode *manager) {
    return (manager == NULL);
}

void Push_CLLL(CLLLNode **manager, void *data) {
    CLLLNode *newCLLLNode = (CLLLNode*)malloc(sizeof(CLLLNode));
    newCLLLNode->value = data;
    newCLLLNode->next = newCLLLNode;
    *manager = newCLLLNode;
}

void InsertAfter_CLLL(CLLLNode *prevCLLLNode, void *data) {
    CLLLNode *newCLLLNode = (CLLLNode*)malloc(sizeof(CLLLNode));
    newCLLLNode->value = data;
    newCLLLNode->next = prevCLLLNode->next;
    prevCLLLNode->next = newCLLLNode;
}

void* Pop_CLLL(CLLLNode **manager) {
    CLLLNode *LLLnodeToDelete = *manager;
    void *poppedValue = LLLnodeToDelete->value;

    *manager = NULL;
    free(LLLnodeToDelete);

    return poppedValue;
}

void* DeleteAfter_CLLL(CLLLNode *prevCLLLNode) {
    CLLLNode *CLLLnodeToDelete = prevCLLLNode->next;
    void *deletedValue = CLLLnodeToDelete->value;

    prevCLLLNode->next = CLLLnodeToDelete->next;
    free(CLLLnodeToDelete);

    return deletedValue;
}