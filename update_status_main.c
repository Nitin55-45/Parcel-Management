#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "status_update.h"
#include "search.h"
#include "hubs.h"
int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("ERROR: Missing arguments\\n");
        return 1;
    }

    char *command = argv[1];

    if (strcmp(command, "deliver") == 0 && argc == 5) {
        char *tracking = argv[2];
        char *driver_id = argv[3];
        char *city = argv[4];

        int ok = save_status_to_csv(tracking, "Delivered", driver_id, "Driver", city);
        if (ok) printf("STATUS_OK\\n");
        else printf("ERROR: Could not save.\\n");
        return 0;
    }

    // --- ADMIN OVERRIDE ACTION ---
    if (strcmp(command, "override") == 0 && argc == 6) {
        char *tracking = argv[2];
        char *status = argv[3];
        char *admin_id = argv[4];
        char *location = argv[5];

        if (!is_valid_status(status)) {
            printf("ERROR: Invalid status\\n");
            return 1;
        }

        int ok = save_status_to_csv(tracking, status, admin_id, "Admin", location);
        if (ok) {
            // Secretly load queues and delete this tracking number so it doesn't accidentally get dispatched later!
            init_hash_table();
            Hub *h = get_or_create_hub(location);
            remove_from_middle(h->booked_queue, tracking);
            remove_from_middle(h->intransit_queue, tracking);
            
            printf("STATUS_OK\\n");
        } else {
            printf("ERROR: Could not save.\\n");
        }
        return 0;
    }

    printf("ERROR: Unknown command.\\n");
    return 1;
}