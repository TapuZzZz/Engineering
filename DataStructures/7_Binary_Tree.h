#ifndef BT_H
#define BT_H

typedef struct BinNode {
    int data;
    struct BinNode* right;
    struct BinNode* left;
} BinNode;


void Init_Tree(BinNode** root);
void Make_Tree(BinNode** root, int data);
void Set_Left(BinNode* parentNode, int data);
void Set_Right(BinNode* parentNode, int data);

#endif 
