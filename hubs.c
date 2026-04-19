#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hubs.h"

Hub* hub_table[TABLE_SIZE];

unsigned int hash_city(const char *city_name) {
    unsigned int hash = 0;
    for (int i = 0; city_name[i] != '\0'; i++) hash = (hash * 31) + city_name[i];
    return hash % TABLE_SIZE;
}

void init_hash_table() {
    for (int i = 0; i < TABLE_SIZE; i++) hub_table[i] = NULL;
}

Queue* create_queue() {
    Queue *q = (Queue*)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    q->count = 0;
    return q;
}

Hub* get_or_create_hub(const char *city_name) {
    unsigned int index = hash_city(city_name);
    Hub *current = hub_table[index];
    while (current != NULL) {
        if (strcasecmp(current->city_name, city_name) == 0) return current;
        current = current->next;
    }
    Hub *new_hub = (Hub*)malloc(sizeof(Hub));
    strcpy(new_hub->city_name, city_name);
    new_hub->booked_queue       = create_queue();
    new_hub->intransit_queue    = create_queue();
    new_hub->out_delivery_queue = create_queue();
    new_hub->next = hub_table[index];
    hub_table[index] = new_hub;
    return new_hub;
}

void enqueue(Queue *q, const char *tracking_number) {
    QueueNode *new_node = (QueueNode*)malloc(sizeof(QueueNode));
    strcpy(new_node->tracking_number, tracking_number);
    new_node->next = NULL;
    if (q->rear == NULL) q->front = q->rear = new_node;
    else { q->rear->next = new_node; q->rear = new_node; }
    q->count++;
}

char* dequeue(Queue *q) {
    if (q->front == NULL) return NULL;
    QueueNode *temp = q->front;
    char *tracking = strdup(temp->tracking_number);
    q->front = q->front->next;
    if (q->front == NULL) q->rear = NULL;
    free(temp);
    q->count--;
    return tracking;
}

int remove_from_middle(Queue *q, const char *tracking_number) {
    if (q->front == NULL) return 0;
    QueueNode *current = q->front, *prev = NULL;
    while (current != NULL) {
        if (strcmp(current->tracking_number, tracking_number) == 0) {
            if (prev == NULL) {
                q->front = current->next;
                if (q->front == NULL) q->rear = NULL;
            } else {
                prev->next = current->next;
                if (current->next == NULL) q->rear = prev;
            }
            free(current);
            q->count--;
            return 1;
        }
        prev = current;
        current = current->next;
    }
    return 0;
}
