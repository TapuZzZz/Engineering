#include <stdio.h>
#include <stdlib.h>

typedef struct CLLLNode {
    int data;
    struct CLLLNode* next;
} CLLLNode;


void Init_CLLL(CLLLNode** last) {
    *last = NULL;
}

int Isempty_CLLL(CLLLNode* last) {
    return (last == NULL);
}

void Push_CLLL(CLLLNode** last, int value) {
    CLLLNode* newCLLLNode = (CLLLNode*)malloc(sizeof(CLLLNode));

    newCLLLNode->data = value;
    newCLLLNode->next = (*last)->next;
    (*last)->next = newCLLLNode;
}

void Insert_after_CLLL(CLLLNode** last, CLLLNode* prevCLLLNode, int value) {
    CLLLNode *newCLLLNode = (CLLLNode *)malloc(sizeof(CLLLNode));

    newCLLLNode->data = value;
    newCLLLNode->next = prevCLLLNode->next;
    prevCLLLNode->next = newCLLLNode;
}

int PopCLLL(CLLLNode** last) {
    CLLLNode* head = (*last)->next;
    int poppedValue = head->data;
    
    (*last)->next = head->next;
    free(head);

    return poppedValue;
}

int Delete_afterCLLL(CLLLNode** last, CLLLNode* prevCLLLNode) {
    CLLLNode* CLLLnodeToDelete = prevCLLLNode->next;
    int deletedValue = CLLLnodeToDelete->data;

    if (CLLLnodeToDelete == *last) {
        *last = prevCLLLNode;
    }

    if (CLLLnodeToDelete == prevCLLLNode) {
        *last = NULL;
    } else {
        prevCLLLNode->next = CLLLnodeToDelete->next;
    }
    free(CLLLnodeToDelete);

    return deletedValue;
}