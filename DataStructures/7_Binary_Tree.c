#include <stdio.h>
#include <stdlib.h>

typedef struct BinNode {
    int data;
    BinNode* right;
    BinNode* left;
} BinNode;

void Init_Tree(BinNode** root) {
    *root = NULL;
}

void Make_Tree(BinNode** root, int data){
    BinNode* newNode = (BinNode*)(malloc(sizeof(BinNode)));

    newNode->data = data;
    newNode->left = NULL;
    newNode->right = NULL;
    
    *root = newNode; 
}


int main(void){
    BinNode* rootPtr;
    Init_Tree(&rootPtr);
    


    return 0;
}