#include <stdio.h>
#include <stdlib.h>
#include "3_LLL.h"
#include "7_BST.h"

void CreateTree(BinNode** root, LLLNode* node) {
    if (node == NULL)
        return;

    if (*root == NULL) {
        Make_Tree(root, node->data);
    } else {
        BinNode** next =
            (node->data < (*root)->data)
            ? &((*root)->left)
            : &((*root)->right);

        CreateTree(next, node);
        return;
    }

    CreateTree(root, node->next);
}

int main(void) {
    int data[] = {41,14,51,36,76,96,23,22,11};
    LLLNode* head;
    Init_LLL(&head);
    Push_LLL(&head, 26);

    for (int i = 0; i < 9; i++){
        Insert_after_LLL(head, data[i]);
    }
    BinNode* root;
    Init_Tree(&root);
    CreateTree(&root, head);

    return 0;
}