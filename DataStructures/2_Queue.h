#ifndef Queue_H
#define Queue_H

typedef struct QueueNode {
    int data;
    struct QueueNode* next;
} QueueNode;

typedef struct Queue {
    struct QueueNode* front;
    struct QueueNode* rear;
    int size;
} Queue;


void Init_queue(Queue* queue);
int Is_Empty(Queue* queue);
void Push_Queue(Queue* queue, int value);
void Insert_Queue(Queue* queue, int value);
int Pop_Queue(Queue* queue);
int Remove_Queue(Queue* queue);

#endif 
