#ifndef STORAGE_H
#define STORAGE_H
#include <stdio.h>

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
    char special_instructions[200];
    int priority;  /* 0 = Normal, 1 = Priority */
    char date[20];
    char time_str[20];
} Parcel;

void save_to_csv(Parcel *p);
void get_current_datetime(char *date, char *time_str);
int  parse_csv_field(const char *line, int pos, char *out, int out_sz);
int  file_exists(const char *filename);

#endif