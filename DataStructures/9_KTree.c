#include <stdio.h>
#include <stdlib.h>
#include "3_LLL.h"


typedef struct KTreeNode {
    void *value;
    LLLNode *children;
} KTreeNode;

void Init_KTree(KTreeNode **root) {
    *root = NULL;
}

void Make_KTree(KTreeNode **root, void *data) {
    KTreeNode *newNode = (KTreeNode*)malloc(sizeof(KTreeNode));
    newNode->value = data;
    newNode->children = NULL;
    *root = newNode;
}

void Add_KChild(KTreeNode *parentNode, void *data) {
    KTreeNode *newNode = (KTreeNode*)malloc(sizeof(KTreeNode));
    newNode->value = data;
    newNode->children = NULL;
    Push_LLL(&parentNode->children, newNode);
}