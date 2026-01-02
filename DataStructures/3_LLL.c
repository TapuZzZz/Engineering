#include <stdio.h>
#include <stdlib.h>

typedef struct LLLNode {
    int data;          
    struct LLLNode* next; 
} LLLNode;


void Init_LLL(LLLNode** head) { 
    *head = NULL; 
}

int Isempty_LLL(LLLNode* head) {
    return (head == NULL);
}

void Push_LLL(LLLNode** head, int value) {
    LLLNode* newLLLNode = (LLLNode*)malloc(sizeof(LLLNode));

    newLLLNode->data = value;
    newLLLNode->next = *head;
    *head = newLLLNode;
}

void Insert_after_LLL(LLLNode* prevLLLNode, int value) {
    LLLNode *newLLLNode = (LLLNode*)malloc(sizeof(LLLNode));

    newLLLNode->data = value;
    newLLLNode->next = prevLLLNode->next;
    prevLLLNode->next = newLLLNode;
}

int Pop_LLL(LLLNode** head) {
    LLLNode* temp = *head;
    int poppedValue = temp->data;
    
    *head = (*head)->next;
    free(temp);

    return poppedValue;
}

int Delete_after_LLL(LLLNode* prevLLLNode) {
    LLLNode* LLLnodeToDelete = prevLLLNode->next;
    int deletedValue = LLLnodeToDelete->data;
    
    prevLLLNode->next = LLLnodeToDelete->next;
    free(LLLnodeToDelete);

    return deletedValue;
}
