#ifndef DLLL_H
#define DLLL_H

typedef struct DLLLNode {
    int data;
    DLLLNode* next; 
    DLLLNode* prev; 
} DLLLNode;


void Init_DLLL(DLLLNode** manager);
int Isempty_DLLL(DLLLNode* manager);
void Push_DLLL(DLLLNode** manager, int value);
void InsertPre_DLLL(DLLLNode* node, int value);
void InsertNext_DLLL();
int Pop_DLLL();
int Delete_DLLL();

#endif 
