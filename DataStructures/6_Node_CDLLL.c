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

void InsertLast_CDLLL(Node **manager, int data){
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = newNode;
    newNode->prev = newNode;
    *manager = newNode;
}

void InsertEnd_CDLLL(Node **manager, int data){
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = (*manager)->next;
    newNode->prev = manager;
    manager = newNode;
}

void InsertPre_DLLL(){
    
}

void InsertNext_DLLL(){
    
}

int Delete_DLLL(Node* nodeToDelete){
    int deletedValue = nodeToDelete->data;

    nodeToDelete->prev->next = nodeToDelete->next;
    nodeToDelete->next->prev = nodeToDelete->prev;
    free(nodeToDelete);

    return deletedValue;
}

int DeleteEnd_DLLL(Node* manager){
    int deletedValue = manager->data;

    manager->prev->next = manager->next;
    manager->next->prev = manager->prev;
    Node* nodeToDelete = manager;
    manager = manager->prev;
    free(nodeToDelete);

    return deletedValue;

}

int DeleteLast_DLLL(Node* manager){
    int deletedValue = manager->data;

    free(manager);
    manager = NULL;

    return deletedValue;
}

int main() {

    return 0;
}