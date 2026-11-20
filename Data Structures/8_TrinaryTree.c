#include <stdio.h>
#include <stdlib.h>

typedef struct TernaryNode {
    void *value;
    struct TernaryNode *left;
    struct TernaryNode *middle;
    struct TernaryNode *right;
} TernaryNode;

void Init_TTree(TernaryNode **root) {
    *root = NULL;
}

void Make_TTree(TernaryNode **root, void *data) {
    TernaryNode *newNode = (TernaryNode*)malloc(sizeof(TernaryNode));
    newNode->value = data;
    newNode->left = NULL;
    newNode->middle = NULL;
    newNode->right = NULL;
    *root = newNode;
}

void Set_TLeft(TernaryNode *parentNode, void *data) {
    TernaryNode *newNode = (TernaryNode*)malloc(sizeof(TernaryNode));
    newNode->value = data;
    newNode->left = NULL;
    newNode->middle = NULL;
    newNode->right = NULL;
    parentNode->left = newNode;
}

void Set_TMiddle(TernaryNode *parentNode, void *data) {
    TernaryNode *newNode = (TernaryNode*)malloc(sizeof(TernaryNode));
    newNode->value = data;
    newNode->left = NULL;
    newNode->middle = NULL;
    newNode->right = NULL;
    parentNode->middle = newNode;
}

void Set_TRight(TernaryNode *parentNode, void *data) {
    TernaryNode *newNode = (TernaryNode*)malloc(sizeof(TernaryNode));
    newNode->value = data;
    newNode->left = NULL;
    newNode->middle = NULL;
    newNode->right = NULL;
    parentNode->right = newNode;
}