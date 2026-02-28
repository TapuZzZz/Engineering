#include <stdio.h>
#include <stdlib.h>

typedef struct DLLLNode {
    void *value;
    DLLLNode *next; 
    DLLLNode *prev; 
} DLLLNode;


void Init_DLLL(DLLLNode **manager) {
    *manager = NULL;
}

int IsEmpty_DLLL(DLLLNode *manager) {
    return (manager == NULL);
}

void Push_DLLL(DLLLNode **manager, void *data) {
    DLLLNode *newDLLLNode = (DLLLNode*)malloc(sizeof(DLLLNode));

    newDLLLNode->value = data;
    newDLLLNode->next = NULL;
    newDLLLNode->prev = NULL;
    *manager = newDLLLNode;
}

void InsertPre_DLLL(DLLLNode *manager, void *data) {
    DLLLNode *newDLLLNode = (DLLLNode*)malloc(sizeof(DLLLNode));

    newDLLLNode->value = data;
    newDLLLNode->prev = manager->prev;
    newDLLLNode->next = manager;
    manager->prev->next = newDLLLNode;
    manager->prev = newDLLLNode;
}

void InsertNext_DLLL(DLLLNode *manager, void *data) {
    DLLLNode *newDLLLNode = (DLLLNode*)malloc(sizeof(DLLLNode));

    newDLLLNode->value = data;
    newDLLLNode->prev = manager;
    newDLLLNode->next = manager->next;
    manager->next->prev = newDLLLNode;
    manager->next = newDLLLNode;
}


int Pop_DLLL(){

}

int Delete_DLLL(){

}