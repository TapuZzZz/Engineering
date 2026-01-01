#include <stdio.h>
#include <stdlib.h>

typedef struct QueueNode {
    int data;
    struct QueueNode* next;
} QueueNode;

typedef struct {
    QueueNode* front;
    QueueNode* rear;
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

void Insert_queue(Queue* queue, int value) {
    QueueNode* newQueueNode = (QueueNode*)malloc(sizeof(QueueNode));
    newQueueNode->data = value;
    newQueueNode->next = NULL;

    if (Is_Empty(queue)) {
        queue->front = newQueueNode;
        queue->rear = newQueueNode;
    } else {
        queue->rear->next = newQueueNode;
        queue->rear = newQueueNode;
    }

    queue->size++;
}

int Remove_queue(Queue *q) {
    QueueNode* temp = q->front;
    int value = temp->data;

    q->front = q->front->next;

    if (q->front == NULL) {
        q->rear = NULL;
    }

    free(temp);
    q->size--;

    return value;
}

int main(void) {

    return 0;
}