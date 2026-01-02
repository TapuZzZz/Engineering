#include <stdio.h>
#include <stdlib.h>

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

