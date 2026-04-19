#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hubs.h"
#include "status_update.h"

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;
    char *target_hub = argv[1];

    init_hash_table();
    Hub *hub = get_or_create_hub(target_hub);

    // 1. Open the Status Log
    FILE *f = fopen("status_log.csv", "r");
    if (!f) return 1;

    char line[512];
    fgets(line, sizeof(line), f); // skip header

    // 2. Read every status line.
    // If the parcel is at our target hub, put it in the right queue!
    while (fgets(line, sizeof(line), f)) {
        char tracking[50], status[50], staff_id[50], staff_name[100], location[100];
        // Parse CSV line
        sscanf(line, "%[^,],%[^,],%[^,],%[^,],%[^\n]", tracking, status, staff_id, staff_name, location);

        // Strip trailing newline/carriage return from location
        location[strcspn(location, "\r\n")] = '\0';

        if (strcmp(location, target_hub) == 0) {
            // Remove from all queues first (handles status updates moving parcel forward)
            remove_from_middle(hub->booked_queue, tracking);
            remove_from_middle(hub->intransit_queue, tracking);
            remove_from_middle(hub->out_delivery_queue, tracking);

            // Enqueue into correct queue based on status
            if (strcmp(status, "Booked") == 0) {
                enqueue(hub->booked_queue, tracking);
            } else if (strcmp(status, "InTransit") == 0) {
                enqueue(hub->intransit_queue, tracking);
            } else if (strcmp(status, "Out for Delivery") == 0) {
                enqueue(hub->out_delivery_queue, tracking);
            }
            // "Delivered" — not enqueued anywhere, removed from all queues above
        }
    }
    fclose(f);

    // 3. Print the queues so Python can read them
    printf("BOOKED:");
    QueueNode *curr = hub->booked_queue->front;
    while (curr) { printf("%s,", curr->tracking_number); curr = curr->next; }

    printf("\nINTRANSIT:");
    curr = hub->intransit_queue->front;
    while (curr) { printf("%s,", curr->tracking_number); curr = curr->next; }

    printf("\nOUTFORDELIVERY:");
    curr = hub->out_delivery_queue->front;
    while (curr) { printf("%s,", curr->tracking_number); curr = curr->next; }

    printf("\n");
    return 0;
}