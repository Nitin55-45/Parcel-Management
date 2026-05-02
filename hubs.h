#ifndef HUBS_H
#define HUBS_H

typedef struct QueueNode {
    char tracking_number[50];
    int priority;  /* 0 = Normal, 1 = Priority */
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *front;
    QueueNode *rear;
    int count;
} Queue;

void  init_queue(Queue *q);
void  enqueue(Queue *q, const char *tracking_number, int priority);
void  enqueue_front(Queue *q, const char *tracking_number);
char* dequeue(Queue *q);

#endif