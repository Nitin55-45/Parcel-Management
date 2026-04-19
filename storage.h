#ifndef STORAGE_H
#define STORAGE_H

#include <stdio.h>

// Define Parcel structure
typedef struct {
    char tracking_number[50];

    char sender_name[100];
    char sender_contact[20];
    char sender_address[200];
    char sender_city[50];
    char receiver_name[100];
    char receiver_contact[20];
    char receiver_address[200];
    char receiver_city[50];
    float weight;
    char parcel_type[50];
    char special_instructions[200]; // can be entered by user
} Parcel;

// Function declarations
void save_to_csv(Parcel *p);
void get_current_datetime(char *date, char *time_str);
void write_csv_row(FILE *f, const Parcel *p, const char *date, const char *time_str);

#endif // STORAGE_H