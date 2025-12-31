#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int data;
    struct Node* next; 
    struct Node* prev; 
} Node;

void Init_CDLLL(Node **manager) {
    *manager = NULL;
}

void InsertFirst_CDLLL(Node **manager, int data){
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = newNode;
    newNode->prev = newNode;
    *manager = newNode;
}

void InsertEnd_CDLLL(Node **manager, int data){
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    
    Node* head = (*manager);
    Node* tail = head->prev;

    newNode->next = head;
    newNode->prev = tail;
    tail->next = newNode;
    head->prev = newNode;
    
    *manager = newNode; 
}

void InsertPre_CDLLL(Node* targetNode, int data){
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    
    newNode->next = targetNode;
    newNode->prev = targetNode->prev;
    
    targetNode->prev->next = newNode;
    targetNode->prev = newNode;
}

void InsertNext_CDLLL(Node* targetNode, int data){
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    
    newNode->next = targetNode->next;
    newNode->prev = targetNode;
    
    targetNode->next->prev = newNode;
    targetNode->next = newNode;
}

int Delete_CDLLL(Node* nodeToDelete){
    int deletedValue = nodeToDelete->data;

    nodeToDelete->prev->next = nodeToDelete->next;
    nodeToDelete->next->prev = nodeToDelete->prev;
    
    free(nodeToDelete);
    return deletedValue;
}

int DeleteEnd_CDLLL(Node **manager){
    Node* toDelete = *manager;
    int deletedValue = toDelete->data;

    toDelete->prev->next = toDelete->next;
    toDelete->next->prev = toDelete->prev;
    
    *manager = toDelete->prev;
    free(toDelete);

    return deletedValue;
}

int DeleteLastOne_CDLLL(Node **manager){
    int deletedValue = (*manager)->data;
    free(*manager);
    *manager = NULL;
    return deletedValue;
}

int main() {

    return 0;
}