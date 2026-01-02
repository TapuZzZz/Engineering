#include <stdio.h>
#include <stdlib.h>

typedef struct QueueNode {
    int data;
    struct QueueNode* next;
} QueueNode;


typedef struct Queue {
    struct QueueNode* front;
    struct QueueNode* rear;
    int size;
} Queue;


void Init_queue(Queue* queue) {
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
}

int Is_Empty(Queue* queue) {
    return queue->size == 0;
}

void Push_Queue(Queue* queue, int value) {
    QueueNode* newQueueNode = (QueueNode*)malloc(sizeof(QueueNode));
    newQueueNode->data = value;
    newQueueNode->next = NULL;

    
    queue->size++;
}

void Insert_Queue(Queue* queue, int value) {
    QueueNode* newQueueNode = (QueueNode*)malloc(sizeof(QueueNode));
    newQueueNode->data = value;
    newQueueNode->next = NULL;

    queue->size++;
}

int Pop_Queue(Queue* queue) {
    QueueNode* temp = queue->front;
    int value = temp->data;

    queue->front = queue->front->next;

    free(temp);
    queue->size--;

    return value;
}

int Remove_Queue(Queue* queue) {
    QueueNode* temp = queue->front;
    int value = temp->data;

    queue->front = queue->front->next;

    free(temp);
    queue->size--;

    return value;
}
