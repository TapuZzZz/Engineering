#include <stdio.h>
#include <stdlib.h>

typedef struct QueueNode {
    void *value;
    struct QueueNode *next;
} QueueNode;

typedef struct Queue {
    struct QueueNode *front;
    struct QueueNode *rear;
} Queue;

void Init_Queue(Queue *queue) {
    queue->front = NULL;
    queue->rear = NULL;
}

int IsEmpty_Queue(Queue *queue) {
    return (queue->rear == queue->front);
}

void Push_Queue(Queue *queue, void *data) {
    QueueNode *newQueueNode = (QueueNode*)malloc(sizeof(QueueNode));
    newQueueNode->value = data;
    newQueueNode->next = NULL;
    queue->rear = newQueueNode;
    queue->front = newQueueNode;
}

void Insert_Queue(Queue *queue, void *data) {
    QueueNode *newQueueNode = (QueueNode*)malloc(sizeof(QueueNode));
    newQueueNode->value = data;
    newQueueNode->next = NULL;
    queue->rear->next = newQueueNode;
    queue->rear = newQueueNode;
}

void *Pop_Queue(Queue *queue) {
    QueueNode *QueueNodeToPop = queue->front;
    void *poppedValue = QueueNodeToPop->value;
    queue->front = NULL;
    queue->rear = NULL;
    free(QueueNodeToPop);
    return poppedValue;
}

void *Remove_Queue(Queue *queue) {
    QueueNode *QueueNodeToRemove = queue->front;
    void *RemovedValue = QueueNodeToRemove->value;
    queue->front = queue->front->next;
    free(QueueNodeToRemove);
    return RemovedValue;
}