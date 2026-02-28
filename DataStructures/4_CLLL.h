#ifndef CLLL_H
#define CLLL_H

typedef struct CLLLNode {
    void *value;
    struct CLLLNode* next;
} CLLLNode;


void Init_CLLL(CLLLNode** last);

int IsEmpty_CLLL(CLLLNode* last);

void Push_CLLL(CLLLNode** last, int value);

void InsertAfter_CLLL(CLLLNode** last, CLLLNode* prevCLLLNode, int value);

void* Pop_CLLL(CLLLNode** last);

void* DeleteAfter_CLLL(CLLLNode *prevCLLLNode);

#endif