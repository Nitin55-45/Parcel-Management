#ifndef STATUS_UPDATE_H
#define STATUS_UPDATE_H

#include <stdio.h>
#include <string.h>
#include <time.h>

#define STATUS_BOOKED    "Booked"
#define STATUS_INTRANSIT "InTransit"
#define STATUS_OUT_FOR_DELIVERY "Out for Delivery"
#define STATUS_DELIVERED "Delivered"

typedef struct StatusNode {
    char tracking_number[50];
    char status[20];
    char staff_id[50];
    char staff_name[100];
    char location[100];
    char date[20];
    char time_str[20];
    struct StatusNode *next;
} StatusNode;

typedef struct {
    StatusNode *head;
    int count;
} StatusLog;

StatusLog* create_status_log();
void       free_status_log(StatusLog *log);
int        append_status(StatusLog *log,
                         const char *tracking, const char *status,
                         const char *staff_id, const char *staff_name,
                         const char *location,
                         const char *date, const char *time_str);
void       get_current_datetime_status(char *date, char *time_str);
int        save_status_to_csv(const char *tracking, const char *status,
                              const char *staff_id, const char *staff_name,
                              const char *location);
int        is_valid_status(const char *status);

#endif