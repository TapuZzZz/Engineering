#ifndef BINARY_TREE_H
#define BINARY_TREE_H

typedef struct BinaryNode {
    void *value;
    struct BinaryNode *left;
    struct BinaryNode *right;
} BinaryNode;

void Init_BTree(BinaryNode **root);

void Make_BTree(BinaryNode **root, void *data);

void Set_BLeft(BinaryNode *parentNode, void *data);

void Set_BRight(BinaryNode *parentNode, void *data);

#endif