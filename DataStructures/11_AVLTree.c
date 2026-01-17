#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int data;
    struct Node *left;
    struct Node *right;
    int height;
} Node;

int getHeight(Node *node) {
    return node == NULL ? 0 : node->height;
}

int getBalance(Node *node) {
    return node == NULL ? 0 : getHeight(node->left) - getHeight(node->right);
}

Node *createNode(int data) {
    Node *node = (Node *)malloc(sizeof(Node));
    node->data = data;
    node->left = NULL;
    node->right = NULL;
    node->height = 1;
    return node;
}

Node *rotateRight(Node *y) {
    Node *x = y->left;
    Node *temp = x->right;
    x->right = y;
    y->left = temp;
    y->height = 1 + (getHeight(y->left) > getHeight(y->right) ? getHeight(y->left) : getHeight(y->right));
    x->height = 1 + (getHeight(x->left) > getHeight(x->right) ? getHeight(x->left) : getHeight(x->right));
    return x;
}

Node *rotateLeft(Node *x) {
    Node *y = x->right;
    Node *temp = y->left;
    y->left = x;
    x->right = temp;
    x->height = 1 + (getHeight(x->left) > getHeight(x->right) ? getHeight(x->left) : getHeight(x->right));
    y->height = 1 + (getHeight(y->left) > getHeight(y->right) ? getHeight(y->left) : getHeight(y->right));
    return y;
}

Node *insert(Node *node, int data) {
    if (node == NULL)
        return createNode(data);
    
    if (data < node->data)
        node->left = insert(node->left, data);
    else if (data > node->data)
        node->right = insert(node->right, data);
    else
        return node;
    
    node->height = 1 + (getHeight(node->left) > getHeight(node->right) ? getHeight(node->left) : getHeight(node->right));
    int balance = getBalance(node);
    
    if (balance > 1 && data < node->left->data)
        return rotateRight(node);
    if (balance < -1 && data > node->right->data)
        return rotateLeft(node);
    if (balance > 1 && data > node->left->data) {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }
    if (balance < -1 && data < node->right->data) {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }
    
    return node;
}

void inorder(Node *node) {
    if (node == NULL) return;
    inorder(node->left);
    printf("%d ", node->data);
    inorder(node->right);
}

int main() {
    Node *root = NULL;
    root = insert(root, 10);
    root = insert(root, 20);
    root = insert(root, 30);
    root = insert(root, 40);
    root = insert(root, 50);
    
    printf("In-order traversal: ");
    inorder(root);
    printf("\n");
    
    return 0;
}