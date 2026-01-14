#include <stdio.h>
#include <stdlib.h>

typedef struct ThreeNode {
    int data;
    struct ThreeNode* left;
    struct ThreeNode* middle;
    struct ThreeNode* right;
} ThreeNode;


void Init_Tree(ThreeNode** root) {
    *root = NULL;
}

void Make_Tree(ThreeNode** root, int data) {
    ThreeNode* newNode = (ThreeNode*)(malloc(sizeof(ThreeNode)));

    newNode->data = data;
    newNode->left = NULL;
    newNode->middle = NULL;
    newNode->right = NULL;
    
    *root = newNode; 
}

void Set_Left(ThreeNode* parentNode, int data) {
    ThreeNode* newNode = (ThreeNode*)(malloc(sizeof(ThreeNode)));

    newNode->data = data;
    newNode->left = NULL;
    newNode->middle = NULL;
    newNode->right = NULL;
    
    parentNode->left = newNode; 
}

void Set_Middle(ThreeNode* parentNode, int data) {
    ThreeNode* newNode = (ThreeNode*)(malloc(sizeof(ThreeNode)));

    newNode->data = data;
    newNode->left = NULL;
    newNode->middle = NULL;
    newNode->right = NULL;
    
    parentNode->middle = newNode; 
}

void Set_Right(ThreeNode* parentNode, int data) {
    ThreeNode* newNode = (ThreeNode*)(malloc(sizeof(ThreeNode)));

    newNode->data = data;
    newNode->left = NULL;
    newNode->middle = NULL;
    newNode->right = NULL;
    
    parentNode->right = newNode; 
}