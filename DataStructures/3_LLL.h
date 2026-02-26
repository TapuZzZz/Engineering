#ifndef LLL_H
#define LLL_H

typedef struct LLLNode {
    void *value;
    struct LLLNode *next;
} LLLNode;

void Init_LLL(LLLNode **head);

int IsEmpty_LLL(LLLNode *head);

void Push_LLL(LLLNode **head, void *data);

void InsertAfter_LLL(LLLNode *prevLLLNode, void *data);

void* Pop_LLL(LLLNode **head);

void* DeleteAfter_LLL(LLLNode *prevLLLNode);

#endif