#include <stdio.h>
#include <string.h>
#include <time.h>
#include "storage.h"

#define FILE_NAME "parcels.csv"

// Check if file exists
int file_exists(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) return 0;
    fclose(f);
    return 1;
}

// Get current date and time separately
void get_current_datetime(char *date, char *time_str) {
    time_t t;
    struct tm *tm_info;
    time(&t);
    tm_info = localtime(&t);
    strftime(date, 20, "%Y-%m-%d", tm_info);
    strftime(time_str, 20, "%H:%M:%S", tm_info);
}

// Write CSV header
void write_csv_header(FILE *f) {
    fprintf(f,
        "tracking_number,sender_name,sender_contact,sender_address,sender_city,"
        "receiver_name,receiver_contact,receiver_address,receiver_city,"
        "weight_kg,parcel_type,special_instructions,date,time\n");
}

// Write one parcel row
void write_csv_row(FILE *f, const Parcel *p, const char *date, const char *time_str) {
    char special[200];

    // Trim newline from user input if present
    strcpy(special, p->special_instructions);
    size_t len = strlen(special);
    if (len > 0 && special[len-1] == '\n') special[len-1] = '\0';

    // If empty after trimming, set to "None"
    if (strlen(special) == 0) {
        strcpy(special, "None");
    }

    fprintf(f,
        "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",%.2f,\"%s\",\"%s\",\"%s\",\"%s\"\n",
        p->tracking_number,
        p->sender_name,
        p->sender_contact,
        p->sender_address,
        p->sender_city, 
        p->receiver_name,
        p->receiver_contact,
        p->receiver_address,
        p->receiver_city,
        p->weight,
        p->parcel_type,
        special,
        date,
        time_str
    );
}

// Save parcel to CSV
void save_to_csv(Parcel *p) {
    int exists = file_exists(FILE_NAME);
    char date[20], time_str[20];
    get_current_datetime(date, time_str);

    FILE *f = fopen(FILE_NAME, "a");
    if (!f) {
        perror("[ERROR] Could not open file");
        return;
    }

    if (!exists) write_csv_header(f);

    write_csv_row(f, p, date, time_str);
    fclose(f);
}