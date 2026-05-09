#ifndef HUBS_H
#define HUBS_H

/* ── Queue Node & Queue (unchanged) ── */
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

/* ── Hub BST — Each city node owns three priority queues ── */
typedef struct HubNode {
    char city_name[50];
    Queue booked_q;
    Queue intransit_q;
    Queue out_for_delivery_q;
    struct HubNode *left, *right;
} HubNode;

/* ── Phone BST — Index of tracking numbers per phone number ── */
typedef struct TrackingLink {
    char tracking_number[50];
    struct TrackingLink *next;
} TrackingLink;

typedef struct PhoneNode {
    char phone[15];
    TrackingLink *parcels;
    int count;
    struct PhoneNode *left, *right;
} PhoneNode;

/* Queue operations */
void  init_queue(Queue *q);
void  enqueue(Queue *q, const char *tracking_number, int priority);
void  enqueue_front(Queue *q, const char *tracking_number);
char* dequeue(Queue *q);
void  free_queue(Queue *q);

/* Hub BST operations */
HubNode* hub_bst_insert(HubNode *root, const char *city);
HubNode* hub_bst_search(HubNode *root, const char *city);
void     hub_bst_free(HubNode *root);

/* Phone BST operations */
PhoneNode* phone_bst_insert(PhoneNode *root, const char *phone, const char *tracking);
PhoneNode* phone_bst_search(PhoneNode *root, const char *phone);
void       phone_bst_free(PhoneNode *root);

#endif