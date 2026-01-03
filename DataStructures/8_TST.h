#ifndef TST_H
#define TST_H

typedef struct ThreeNode {
    int data;
    struct ThreeNode* right;
    struct ThreeNode* middle;
    struct ThreeNode* left;
} ThreeNode;


void Init_Tree(ThreeNode** root);
void Make_Tree(ThreeNode** root, int data);
void Set_Left(ThreeNode* parentNode, int data);
void Set_Middle(ThreeNode* parentNode, int data);
void Set_Right(ThreeNode* parentNode, int data);

#endif