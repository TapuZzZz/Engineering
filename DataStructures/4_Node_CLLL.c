#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int data;
    struct Node* next;
} Node;

void Init_CLLL(Node **last) {
    *last = NULL;
}

int Isempty_CLLL(Node *last) {
    return (last == NULL);
}

void Push_CLLL(Node **last, int value) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (!newNode) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    newNode->data = value;
    newNode->next = (*last)->next;
    (*last)->next = newNode;
}

void Insert_after_CLLL(Node **last, Node *prevNode, int value) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (!newNode) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    newNode->data = value;
    newNode->next = prevNode->next;
    prevNode->next = newNode;
}

int PopCLLL(Node **last) {
    Node *head = (*last)->next;
    int poppedValue = head->data;
    
    (*last)->next = head->next;
    free(head);

    return poppedValue;
}

int Delete_afterCLLL(Node **last, Node *prevNode) {
    Node *nodeToDelete = prevNode->next;
    int deletedValue = nodeToDelete->data;

    if (nodeToDelete == *last) {
        *last = prevNode;
    }

    if (nodeToDelete == prevNode) {
        *last = NULL;
    } else {
        prevNode->next = nodeToDelete->next;
    }
    free(nodeToDelete);

    return deletedValue;
}

int main(void) {
    
    return 0;
}
