#ifndef KTREE_H
#define KTREE_H

#include "3_LLL.h"

typedef struct KTreeNode {
    void *value;
    LLLNode *children;
} KTreeNode;

void Init_KTree(KTreeNode **root);
void Make_KTree(KTreeNode **root, void *data);
void Add_KChild(KTreeNode *parentNode, void *data);

#endif