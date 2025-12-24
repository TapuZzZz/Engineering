#include <stdio.h>

#define MAX 100

typedef struct {
    int data[MAX];
    int front;
    int rear;
    int size;
} Queue;

void Init_queue(Queue *q) {
    q->front = 0;
    q->rear = 0;
    q->size = 0;
}

int Is_Queue_Empty(Queue *q) {
    return (q->size == 0);
}

void Insert_queue(Queue *q, int value) {
    q->data[q->rear] = value;
    q->rear = (q->rear + 1) % MAX;
    q->size++;
}

void Remove_queue(Queue *q, int *value) {
    *value = q->data[q->front];
    q->front = (q->front + 1) % MAX;
    q->size--;

}

int main(void) {

    return 0;
}
