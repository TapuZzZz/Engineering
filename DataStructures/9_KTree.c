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
    KTreeNode *newKTreeNode = (KTreeNode*)malloc(sizeof(KTreeNode));
    newKTreeNode->value = data;
    newKTreeNode->children = NULL;
    *root = newKTreeNode;
}

void Add_KChild(KTreeNode *parentNode, void *data) {
    KTreeNode *newKTreeNode = (KTreeNode*)malloc(sizeof(KTreeNode));
    newKTreeNode->value = data;
    newKTreeNode->children = NULL;
    Push_LLL(&parentNode->children, newKTreeNode);
}