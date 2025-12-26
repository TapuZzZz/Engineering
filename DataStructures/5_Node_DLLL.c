#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int data;
    struct Node* next; 
    struct Node* prev; 
} Node;

void Init_DLLL(Node** manager) {
    *manager = NULL;
}

void Push_DLLL(Node** manager, int value) {
    Node* newNode = (Node *)malloc(sizeof(Node));
    if (!newNode) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    newNode->data = value;
    newNode->next = NULL;
    newNode->prev = NULL;
    *manager = newNode;

}

void InsertPre_DLLL(Node* node, int value){
    Node *newNode = (Node *)malloc(sizeof(Node));
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


int main() {

    return 0;
}
