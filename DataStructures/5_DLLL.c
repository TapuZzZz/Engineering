#include <stdio.h>
#include <stdlib.h>

typedef struct DLLLNode {
    int data;
    DLLLNode* next; 
    DLLLNode* prev; 
} DLLLNode;

void Init_DLLL(DLLLNode** manager) {
    *manager = NULL;
}

void Push_DLLL(DLLLNode** manager, int value) {
    DLLLNode* newNode = (DLLLNode *)malloc(sizeof(DLLLNode));
    if (!newNode) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    newNode->data = value;
    newNode->next = NULL;
    newNode->prev = NULL;
    *manager = newNode;
}

void InsertPre_DLLL(DLLLNode* node, int value){
    DLLLNode *newNode = (DLLLNode *)malloc(sizeof(DLLLNode));
    if (!newNode) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    newNode->prev = node->prev;
    newNode->next = node;

    node->prev->next = newNode;
    node->prev = newNode;
}

void InsertNext_DLLL(){
    
}


int Pop_DLLL(){

}

int Delete_DLLL(){

}

