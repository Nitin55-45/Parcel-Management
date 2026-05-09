#define _CRT_SECURE_NO_WARNINGS
/* hubs.c — Priority Queue + Hub BST + Phone BST */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hubs.h"

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

/* ══════════════════════════════════════════════════════════════
   QUEUE OPERATIONS (unchanged logic)
   ══════════════════════════════════════════════════════════════ */

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

void free_queue(Queue *q) {
    QueueNode *c = q->front, *t;
    while (c) { t = c->next; free(c); c = t; }
    q->front = q->rear = NULL; q->count = 0;
}

/* ══════════════════════════════════════════════════════════════
   HUB BST OPERATIONS
   Each city gets its own node with 3 embedded priority queues.
   ══════════════════════════════════════════════════════════════ */

HubNode* hub_bst_insert(HubNode *root, const char *city) {
    if (!root) {
        HubNode *n = calloc(1, sizeof(HubNode));
        strncpy(n->city_name, city, 49); n->city_name[49] = '\0';
        init_queue(&n->booked_q);
        init_queue(&n->intransit_q);
        init_queue(&n->out_for_delivery_q);
        n->left = n->right = NULL;
        return n;
    }
    int c = strcasecmp(city, root->city_name);
    if (c < 0) root->left = hub_bst_insert(root->left, city);
    else if (c > 0) root->right = hub_bst_insert(root->right, city);
    /* if c == 0, city already exists — return as-is */
    return root;
}

HubNode* hub_bst_search(HubNode *root, const char *city) {
    if (!root) return NULL;
    int c = strcasecmp(city, root->city_name);
    return (c == 0) ? root : hub_bst_search(c < 0 ? root->left : root->right, city);
}

void hub_bst_free(HubNode *root) {
    if (!root) return;
    hub_bst_free(root->left);
    hub_bst_free(root->right);
    free_queue(&root->booked_q);
    free_queue(&root->intransit_q);
    free_queue(&root->out_for_delivery_q);
    free(root);
}

/* ══════════════════════════════════════════════════════════════
   PHONE BST OPERATIONS
   Each phone number gets a linked list of associated tracking numbers.
   ══════════════════════════════════════════════════════════════ */

PhoneNode* phone_bst_insert(PhoneNode *root, const char *phone, const char *tracking) {
    if (!root) {
        PhoneNode *n = calloc(1, sizeof(PhoneNode));
        strncpy(n->phone, phone, 14); n->phone[14] = '\0';
        TrackingLink *tl = malloc(sizeof(TrackingLink));
        strncpy(tl->tracking_number, tracking, 49); tl->tracking_number[49] = '\0';
        tl->next = NULL;
        n->parcels = tl; n->count = 1;
        n->left = n->right = NULL;
        return n;
    }
    int c = strcmp(phone, root->phone);
    if (c < 0) root->left = phone_bst_insert(root->left, phone, tracking);
    else if (c > 0) root->right = phone_bst_insert(root->right, phone, tracking);
    else {
        /* Phone exists — add tracking number (avoid duplicates) */
        TrackingLink *cur = root->parcels;
        while (cur) {
            if (!strcmp(cur->tracking_number, tracking)) return root;
            cur = cur->next;
        }
        TrackingLink *tl = malloc(sizeof(TrackingLink));
        strncpy(tl->tracking_number, tracking, 49); tl->tracking_number[49] = '\0';
        tl->next = root->parcels;
        root->parcels = tl; root->count++;
    }
    return root;
}

PhoneNode* phone_bst_search(PhoneNode *root, const char *phone) {
    if (!root) return NULL;
    int c = strcmp(phone, root->phone);
    return (c == 0) ? root : phone_bst_search(c < 0 ? root->left : root->right, phone);
}

void phone_bst_free(PhoneNode *root) {
    if (!root) return;
    phone_bst_free(root->left);
    phone_bst_free(root->right);
    TrackingLink *c = root->parcels, *t;
    while (c) { t = c->next; free(c); c = t; }
    free(root);
}
