#ifndef CDLLL_H
#define CDLLL_H

typedef struct CDLLLNode {
    int data;
    CDLLLNode* next; 
    CDLLLNode* prev; 
} CDLLLNode;


void Init_CDLLL(CDLLLNode** manager);

void InsertFirst_CDLLL(CDLLLNode** manager, int data);
void InsertEnd_CDLLL(CDLLLNode** manager, int data);
void InsertPre_CDLLL(CDLLLNode* targetNode, int data);
void InsertNext_CDLLL(CDLLLNode* targetNode, int data);
int Delete_CDLLL(CDLLLNode* nodeToDelete);
int DeleteEnd_CDLLL(CDLLLNode **manager);
int DeleteLastOne_CDLLL(CDLLLNode** manager);

#endif 
