#include <stdio.h>
#include <stdlib.h>

typedef struct BinaryNode {
    void *value;
    struct BinaryNode *left;
    struct BinaryNode *right;
} BinaryNode;

void Init_BTree(BinaryNode **root) {
    *root = NULL;
}

void Make_BTree(BinaryNode **root, void *data) {
    BinaryNode *newNode = (BinaryNode*)malloc(sizeof(BinaryNode));
    newNode->value = data;
    newNode->left = NULL;
    newNode->right = NULL;
    *root = newNode;
}

void Set_BLeft(BinaryNode *parentNode, void *data) {
    BinaryNode *newNode = (BinaryNode*)malloc(sizeof(BinaryNode));
    newNode->value = data;
    newNode->left = NULL;
    newNode->right = NULL;
    parentNode->left = newNode;
}

void Set_BRight(BinaryNode *parentNode, void *data) {
    BinaryNode *newNode = (BinaryNode*)malloc(sizeof(BinaryNode));
    newNode->value = data;
    newNode->left = NULL;
    newNode->right = NULL;
    parentNode->right = newNode;
}