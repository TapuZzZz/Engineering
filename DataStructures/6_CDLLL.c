#include <stdio.h>
#include <stdlib.h>

typedef struct CDLLLNode {
    int data;
    CDLLLNode* next; 
    CDLLLNode* prev; 
} CDLLLNode;

void Init_CDLLL(CDLLLNode** manager) {
    *manager = NULL;
}

void InsertFirst_CDLLL(CDLLLNode** manager, int data){
    CDLLLNode* newNode = (CDLLLNode*)malloc(sizeof(CDLLLNode));
    newNode->data = data;
    newNode->next = newNode;
    newNode->prev = newNode;
    *manager = newNode;
}

void InsertEnd_CDLLL(CDLLLNode** manager, int data){
    CDLLLNode* newNode = (CDLLLNode*)malloc(sizeof(CDLLLNode));
    newNode->data = data;
    
    CDLLLNode* head = (*manager);
    CDLLLNode* tail = head->prev;

    newNode->next = head;
    newNode->prev = tail;
    tail->next = newNode;
    head->prev = newNode;
    
    *manager = newNode; 
}

void InsertPre_CDLLL(CDLLLNode* targetNode, int data){
    CDLLLNode* newNode = (CDLLLNode*)malloc(sizeof(CDLLLNode));
    newNode->data = data;
    
    newNode->next = targetNode;
    newNode->prev = targetNode->prev;
    
    targetNode->prev->next = newNode;
    targetNode->prev = newNode;
}

void InsertNext_CDLLL(CDLLLNode* targetNode, int data){
    CDLLLNode* newNode = (CDLLLNode*)malloc(sizeof(CDLLLNode));
    newNode->data = data;
    
    newNode->next = targetNode->next;
    newNode->prev = targetNode;
    
    targetNode->next->prev = newNode;
    targetNode->next = newNode;
}

int Delete_CDLLL(CDLLLNode* nodeToDelete){
    int deletedValue = nodeToDelete->data;

    nodeToDelete->prev->next = nodeToDelete->next;
    nodeToDelete->next->prev = nodeToDelete->prev;
    
    free(nodeToDelete);
    return deletedValue;
}

int DeleteEnd_CDLLL(CDLLLNode **manager){
    CDLLLNode* toDelete = *manager;
    int deletedValue = toDelete->data;

    toDelete->prev->next = toDelete->next;
    toDelete->next->prev = toDelete->prev;
    
    *manager = toDelete->prev;
    free(toDelete);

    return deletedValue;
}

int DeleteLastOne_CDLLL(CDLLLNode** manager){
    int deletedValue = (*manager)->data;
    free(*manager);
    *manager = NULL;
    return deletedValue;
}
