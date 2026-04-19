#include <stdio.h>
#include <string.h>
#include "search.h"

int main() {
    char tracking[50];
    
    // Read the tracking number passed from Python
    if (fgets(tracking, sizeof(tracking), stdin)) {
        tracking[strcspn(tracking, "\n")] = 0; // Remove newline
    }

    // 1. Build the Binary Search Tree
    ParcelNode *root = load_parcels_to_bst("parcels.csv");
    if (root == NULL) {
        printf("ERROR: Could not load database.\n");
        return 1;
    }

    // 2. Search instantly using O(log N)
    ParcelNode *result = search_bst(root, tracking);

    // 3. Print result for Python to read
    if (result != NULL) {
        printf("FOUND\n");
        printf("%s|%s|%s|%s|%s|%s|%s|%s|%s|%.2f|%s|%s\n",
            result->data.tracking_number, result->data.sender_name, result->data.sender_contact,
            result->data.sender_address, result->data.sender_city, result->data.receiver_name,
            result->data.receiver_contact, result->data.receiver_address, result->data.receiver_city,
            result->data.weight, result->data.parcel_type, result->data.special_instructions);
    } else {
        printf("NOT_FOUND\n");
    }

    free_bst(root);
    return 0;
}