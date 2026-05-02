#define _CRT_SECURE_NO_WARNINGS
/* hubs.c — Priority Queue (Linked List) for parcel tracking */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hubs.h"

void init_queue(Queue *q) { q->front = q->rear = NULL; q->count = 0; }

/* Priority-aware enqueue:
   - Priority (1): Inserted AFTER existing priority parcels, BEFORE normal parcels
   - Normal   (0): Inserted at the end of the queue
   This maintains FIFO within each priority tier. */
void enqueue(Queue *q, const char *trk, int prio) {
    QueueNode *n = malloc(sizeof(QueueNode));
    strncpy(n->tracking_number, trk, 49); n->tracking_number[49] = '\0';
    n->priority = prio; n->next = NULL;

    if (!q->front) { q->front = q->rear = n; }
    else if (prio == 1) {
        /* Case 1: First node is Normal — insert at front */
        if (q->front->priority == 0) { n->next = q->front; q->front = n; }
        else {
            /* Case 2: Walk past existing priority nodes, insert after last one */
            QueueNode *c = q->front;
            while (c->next && c->next->priority == 1) c = c->next;
            n->next = c->next; c->next = n;
            if (!n->next) q->rear = n;
        }
    } else {
        /* Normal: always at the end */
        q->rear->next = n; q->rear = n;
    }
    q->count++;
}

/* Admin backward-override: absolute front of queue, ignores priority tiers */
void enqueue_front(Queue *q, const char *trk) {
    QueueNode *n = malloc(sizeof(QueueNode));
    strncpy(n->tracking_number, trk, 49); n->tracking_number[49] = '\0';
    n->priority = 1; n->next = q->front;
    q->front = n;
    if (!q->rear) q->rear = n;
    q->count++;
}

char* dequeue(Queue *q) {
    if (!q->front) return NULL;
    QueueNode *tmp = q->front;
    char *trk = malloc(50);
    if (trk) strcpy(trk, tmp->tracking_number);
    q->front = q->front->next;
    if (!q->front) q->rear = NULL;
    free(tmp); q->count--;
    return trk;
}
