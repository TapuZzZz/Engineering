#ifndef LLL_H
#define LLL_H

typedef struct LLLNode {
    int data;
    struct LLLNode* next;
} LLLNode;


void Init_LLL(LLLNode** head);
int Isempty_LLL(LLLNode* head);
void Push_LLL(LLLNode** head, int value);
void Insert_after_LLL(LLLNode* prevLLLNode, int value);
int Pop_LLL(LLLNode** head);
int Delete_after_LLL(LLLNode* prevLLLNode);

#endif