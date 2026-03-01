#ifndef LLL.H
#define LLL.H

typedef struct LLLNode {
    void *value;
    struct LLLNode *next;
} LLLNode;

void Init_LLL(LLLNode **head);

int IsEmpty_LLL(LLLNode *head);

void Push_LLL(LLLNode **head, void *data);

void InsertAfter_LLL(LLLNode *prev, void *data);

void* Pop_LLL(LLLNode **head);

void* DeleteAfter_LLL(LLLNode *manager);

#endif