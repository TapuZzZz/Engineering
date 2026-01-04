#include <stdio.h>
#include <stdlib.h>
#include "7_BST.h"
#include "8_TST.h"

// T(n) = 3T(n/3) + O(1)
// O(n)
int Task1(ThreeNode* root) {
    if (root == NULL) return 0;

    if (!root->left && root->middle && !root->right){
        return 1 + Task1(root->left) + Task1(root->middle) + Task1(root->right);
    }

    if (root->left && !root->middle && !root->right){
        return -1 + Task1(root->left) + Task1(root->middle) + Task1(root->right);
    }

    return Task1(root->left) + Task1(root->middle) + Task1(root->right);
}

// T(n) = 2T(n/2) + O(1)
// O(n)
int Task2a(ThreeNode* root) {
    if (root == NULL) return 1;

    if (root->left && root->left->data >= root->data) return 0;

    if (root->right && root->right->data <= root->data) return 0;

    return Task2a(root->left) && Task2a(root->right);
}

// T(n) = 3T(n/3) + O(1)
// O(n)
// היה צריך לעשות יותר בדיקות, הדמיון זהה רק בדיקה נוספת עבר התו האמצעי
int Task2b(ThreeNode* root) {
    if (root == NULL) return 1;

    if (root->left && root->left->data > root->data) 
        return 0;

    if (root->right && root->right->data < root->data) 
        return 0;

    if (root->middle && root->left && root->middle->data <= root->left->data)
        return 0;

    if (root->middle && root->right && root->middle->data >= root->right->data)
        return 0;

    return Task2b(root->left) && Task2b(root->middle) && Task2b(root->right);
}

// T(n) = 2T(n/2) + O(1)
// O(n)
int Task3(ThreeNode* root1, ThreeNode* root2) {
    if (!root1 && !root2) return 1;

    if (root1 && !root2) return 0;
    if (!root1 && root2) return 0;
    
    return Task3(root1->left, root2->left) && Task3(root1->right, root2->right);
}

// T(n) = 
// O(n)
ThreeNode* Task4(ThreeNode** root) {
    if (root == NULL) return NULL;
}

// T(n) = 
// O(n)
ThreeNode* Task5(ThreeNode* root, ThreeNode* node) {
    ThreeNode* temp = NULL;
    
    if (root == NULL) return NULL;

    if (root->left == node) return root->right;
    if (root->right == node) return root->left;

    temp = Task5(root->left, node);
    temp == NULL ? temp = Task5(root->right, node) : 0;

    return temp;
}

// T(n) = 
// O(n)
int Task6(ThreeNode* root) {
    if (root = NULL) return 0;
    
}

// T(n) = 
// O(n)
int Task7(ThreeNode* root) {
    if (root = NULL) return 0;
    
}

// T(n) = 
// O(n)
int Task8(ThreeNode* root) {
    if (root = NULL) return 0;
    
}