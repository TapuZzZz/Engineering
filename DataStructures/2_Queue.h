#ifndef Queue_H
#define Queue_H

typedef struct QueueNode {
    void *value;
    struct QueueNode *next;
} QueueNode;

typedef struct Queue {
    struct QueueNode *front;
    struct QueueNode *rear;
    unsigned int size;
} Queue;

void Init_queue(Queue *queue);

int Is_Empty(Queue *queue);

void Push_Queue(Queue* queue, void *data);

void Insert_Queue(Queue* queue, void *data);

void* Pop_Queue(Queue* queue);

void* Remove_Queue(Queue* queue);

#endif