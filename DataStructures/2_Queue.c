#include <stdio.h>
#include <stdlib.h>

typedef struct QueueNode {
    void *value;
    struct QueueNode *next;
} QueueNode;

typedef struct Queue {
    struct QueueNode *front;
    struct QueueNode *rear;
    unsigned int size;
} Queue;

void Init_queue(Queue *queue) {
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
}

int Is_Empty(Queue *queue) {
    return (queue->size == 0);
}

void Push_Queue(Queue *queue, void *data) {
    QueueNode* newQueueNode = (QueueNode*)malloc(sizeof(QueueNode));
    newQueueNode->value = data;
    newQueueNode->next = NULL;

    queue->rear = newQueueNode;
    queue->front = newQueueNode;
    queue->size++;
}

void Insert_Queue(Queue *queue, void *data) {
    QueueNode* newQueueNode = (QueueNode*)malloc(sizeof(QueueNode));
    newQueueNode->value = data;
    newQueueNode->next = NULL;

    queue->rear->next = newQueueNode;
    queue->rear = newQueueNode;
    queue->size++;
}

void* Pop_Queue(Queue *queue) {
    QueueNode* temp = queue->front;
    void* poppedValue = temp->value;

    queue->front = NULL;
    queue->rear = NULL;
    free(temp);
    queue->size--;

    return poppedValue;
}

void* Remove_Queue(Queue *queue) {
    QueueNode* temp = queue->front;
    void* poppedValue = temp->value;

    queue->front = queue->front->next;
    free(temp);
    queue->size--;

    return poppedValue;
}