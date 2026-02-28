#ifndef TERNARY_TREE_H
#define TERNARY_TREE_H

typedef struct TernaryNode {
    void *value;
    struct TernaryNode *left;
    struct TernaryNode *middle;
    struct TernaryNode *right;
} TernaryNode;

void Init_TTree(TernaryNode **root);

void Make_TTree(TernaryNode **root, void *data);

void Set_TLeft(TernaryNode *parentNode, void *data);

void Set_TMiddle(TernaryNode *parentNode, void *data);

void Set_TRight(TernaryNode *parentNode, void *data);

#endif