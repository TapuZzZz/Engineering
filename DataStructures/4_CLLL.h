#ifndef CLLL_H
#define CLLL_H

typedef struct CLLLNode {
    int data;
    struct CLLLNode* next;
} CLLLNode;


void Init_CLLL(CLLLNode** last);
int Isempty_CLLL(CLLLNode* last);
void Push_CLLL(CLLLNode** last, int value);
void Insert_after_CLLL(CLLLNode** last, CLLLNode* prevCLLLNode, int value);
int PopCLLL(CLLLNode** last);
int Delete_afterCLLL(CLLLNode** last, CLLLNode* prevCLLLNode);

#endif 
