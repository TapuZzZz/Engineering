#ifndef QUEUE.H
#define QUEUE.H

typedef struct QueueNode {
    void *value;
    struct QueueNode *next;
} QueueNode;

typedef struct Queue {
    struct QueueNode *front;
    struct QueueNode *rear;
} Queue;

void Init_Queue(Queue *queue);

int IsEmpty_Queue(Queue *queue);

void Push_Queue(Queue *queue, void *data);

void Insert_Queue(Queue *queue, void *data);

void* Pop_Queue(Queue *queue);

void* Remove_Queue(Queue *queue);

#endif