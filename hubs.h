#ifndef HUBS_H
#define HUBS_H

#define TABLE_SIZE 50

typedef struct QueueNode {
    char tracking_number[50];
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *front;
    QueueNode *rear;
    int count;
} Queue;

typedef struct Hub {
    char city_name[50];
    Queue *booked_queue;
    Queue *intransit_queue;
    Queue *out_delivery_queue;
    struct Hub *next;
} Hub;

void init_hash_table();
Hub* get_or_create_hub(const char *city_name);
void enqueue(Queue *q, const char *tracking_number);
char* dequeue(Queue *q);
int remove_from_middle(Queue *q, const char *tracking_number);

#endif // HUBS_H