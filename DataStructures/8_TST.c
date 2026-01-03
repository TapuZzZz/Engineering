#include <stdio.h>
#include <stdlib.h>

typedef struct ThreeNode {
    int data;
    struct ThreeNode* right;
    struct ThreeNode* middle;
    struct ThreeNode* left;
} ThreeNode;