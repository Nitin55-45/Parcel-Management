#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "details.h"
#include "tracking.h"
#include "storage.h"
#include "status_update.h"

int main() {
    Person sender, receiver;
    ParcelInfo parcel;
    char tracking_number[30];
    Parcel p;

    printf("\n==============================\n");
    printf("    PARCEL BOOKING SYSTEM     \n");
    printf("==============================\n");

    // Get sender and receiver info
    input_person(&sender, "Sender");
    input_person(&receiver, "Receiver");
    input_parcel_info(&parcel);   // includes weight, type, and optional specialInstructions

    // Generate tracking number
    generate_tracking_number(tracking_number);
    strncpy(p.tracking_number, tracking_number, sizeof(p.tracking_number)-1);

    // Fill sender details
    strncpy(p.sender_name, sender.name, sizeof(p.sender_name)-1);
    strncpy(p.sender_contact, sender.phone, sizeof(p.sender_contact)-1);
    strncpy(p.sender_address, sender.address, sizeof(p.sender_address)-1);
    strncpy(p.sender_city, sender.city, sizeof(p.sender_city)-1); 
    // Fill receiver details
    strncpy(p.receiver_name, receiver.name, sizeof(p.receiver_name)-1);
    strncpy(p.receiver_contact, receiver.phone, sizeof(p.receiver_contact)-1);
    strncpy(p.receiver_address, receiver.address, sizeof(p.receiver_address)-1);
    strncpy(p.receiver_city, receiver.city, sizeof(p.receiver_city)-1);
    // Parcel details
    p.weight = parcel.weight;
    strncpy(p.parcel_type, parcel.type, sizeof(p.parcel_type)-1);

    // Special instructions
    if (strlen(parcel.specialInstructions) > 1) {
        strncpy(p.special_instructions, parcel.specialInstructions, sizeof(p.special_instructions)-1);
    } else {
        strncpy(p.special_instructions, "None", sizeof(p.special_instructions)-1);
    }

    // Save to CSV
    save_to_csv(&p);
    save_status_to_csv(tracking_number, "Booked", "SYS01", "System Auto", sender.city);
    char date[20],time_str[20];
    get_current_datetime(date,time_str);

    // Print confirmation
    printf("\n==============================\n");
    printf("       BOOKING CONFIRMED      \n");
    printf("==============================\n");
    printf("  Tracking No      : %s\n", tracking_number);
    printf("  Sender           : %s\n", sender.name);
    printf("  Receiver         : %s\n", receiver.name);
    printf("  Weight           : %.2f kg\n", parcel.weight);
    printf("  Parcel Type      : %s\n", parcel.type);
    printf("  Special Instr.   : %s\n", p.special_instructions);
    printf("  Date & Time      : %s %s\n", date, time_str);
    printf("  Saved to         : parcels.csv\n");
    printf("==============================\n\n");

    return 0;
}
