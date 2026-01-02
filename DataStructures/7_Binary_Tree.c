#include <stdio.h>
#include <stdlib.h>
#include "3_LLL.h"

typedef struct BinNode {
    int data;
    struct BinNode* right;
    struct BinNode* left;
} BinNode;

void Init_Tree(BinNode** root) {
    *root = NULL;
}

void Make_Tree(BinNode** root, int data) {
    BinNode* newNode = (BinNode*)(malloc(sizeof(BinNode)));

    newNode->data = data;
    newNode->left = NULL;
    newNode->right = NULL;
    
    *root = newNode; 
}

void Set_Left(BinNode* parentNode, int data) {
    BinNode* newNode = (BinNode*)(malloc(sizeof(BinNode)));

    newNode->data = data;
    newNode->left = NULL;
    newNode->right = NULL;
    
    parentNode->left = newNode; 
}

void Set_Right(BinNode* parentNode, int data) {
    BinNode* newNode = (BinNode*)(malloc(sizeof(BinNode)));

    newNode->data = data;
    newNode->left = NULL;
    newNode->right = NULL;
    
    parentNode->right = newNode; 
}

void CreateTree(BinNode** root, int data) {
    if (*root == NULL) {
        Make_Tree(root, data);
        return;
    }

    BinNode** next = (data < (*root)->data) ? &((*root)->left) : &((*root)->right);

    if (*next == NULL)
        (data < (*root)->data) ? Set_Left(*root, data) : Set_Right(*root, data);
    else
        CreateTree(next, data);
}

int main(void) {
    int data[] = {41,14,51,36,76,96,23,22,11};
    int n = sizeof(data) / sizeof(data[0]);

    BinNode* root = NULL;

    for (int i = 0; i < n; i++) {
        CreateTree(&root, data[i]);
    }

    return 0;
}
