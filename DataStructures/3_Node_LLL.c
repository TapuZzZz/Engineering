#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int data;          
    struct Node* next; 
} Node;

void Init_LLL(Node **head) { 
    *head = NULL; 
}

int Isempty_LLL(Node *head) {
    return (head == NULL);
}

void Push_LLL(Node **head, int value) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        perror("Error: Memory allocation failed in Push_LLL");
        exit(EXIT_FAILURE); 
    }

    newNode->data = value;
    newNode->next = *head;
    *head = newNode;
}

void Insert_after_LLL(Node *prevNode, int value) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        perror("Error: Memory allocation failed in Insert_after_LLL");
        exit(EXIT_FAILURE);
    }

    newNode->data = value;
    newNode->next = prevNode->next;
    prevNode->next = newNode;
}

int Pop_LLL(Node **head) {
    Node *temp = *head;
    int poppedValue = temp->data;
    
    *head = (*head)->next;
    free(temp);

    return poppedValue;
}

int Delete_after_LLL(Node *prevNode) {
    Node *nodeToDelete = prevNode->next;
    int deletedValue = nodeToDelete->data;
    
    prevNode->next = nodeToDelete->next;
    free(nodeToDelete);

    return deletedValue;
}

int main(void) {
    
    return 0;
}