#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int data;
    struct Node *left;
    struct Node *right;
    int height;
} Node;

Node* CreateNode(int data) {
    Node *newNode = (Node*)malloc(sizeof(Node));

    newNode->data = data;
    newNode->left = NULL;
    newNode->right = NULL;
    newNode->height = 1;

    return newNode;
}

int GetHeight(Node *node) {
    if (node == NULL)
        return 0;

    return node->height;
}

int Max(int a, int b) {
    return (a > b) ? a : b;
}

int GetBalance(Node *node) {
    if (node == NULL)
        return 0;

    return GetHeight(node->left) - GetHeight(node->right);
}

Node* RightRotate(Node *y) {

    Node *x = y->left;
    Node *T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = 1 + Max(
        GetHeight(y->left),
        GetHeight(y->right)
    );

    x->height = 1 + Max(
        GetHeight(x->left),
        GetHeight(x->right)
    );

    return x;
}

Node* LeftRotate(Node *x) {

    Node *y = x->right;
    Node *T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = 1 + Max(
        GetHeight(x->left),
        GetHeight(x->right)
    );

    y->height = 1 + Max(
        GetHeight(y->left),
        GetHeight(y->right)
    );

    return y;
}

Node* Insert(Node *root, int data) {

    if (root == NULL)
        return CreateNode(data);

    if (data < root->data)
        root->left = Insert(root->left, data);

    else if (data > root->data)
        root->right = Insert(root->right, data);

    else
        return root; 

    root->height = 1 + Max(
        GetHeight(root->left),
        GetHeight(root->right)
    );

    int balance = GetBalance(root);

    /* LL */
    if (balance > 1 && data < root->left->data)
        return RightRotate(root);

    /* RR */
    if (balance < -1 && data > root->right->data)
        return LeftRotate(root);

    /* LR */
    if (balance > 1 && data > root->left->data) {
        root->left = LeftRotate(root->left);
        return RightRotate(root);
    }

    /* RL */
    if (balance < -1 && data < root->right->data) {
        root->right = RightRotate(root->right);
        return LeftRotate(root);
    }

    return root;
}

void InOrder(Node *root) {
    if (root == NULL)
        return;

    InOrder(root->left);
    printf("%d ", root->data);
    InOrder(root->right);
}

int main() {

    Node *root = NULL;

    root = Insert(root, 30);
    root = Insert(root, 20);
    root = Insert(root, 10);

    printf("InOrder: ");
    InOrder(root);

    printf("\nRoot = %d\n", root->data);
    printf("Height = %d\n", root->height);

    return 0;
}