#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "status_update.h"

#define STATUS_LOG_FILE "status_log.csv"

StatusLog* create_status_log() {
    StatusLog *log = (StatusLog*)malloc(sizeof(StatusLog));
    if (!log) return NULL;
    log->head  = NULL;
    log->count = 0;
    return log;
}

void free_status_log(StatusLog *log) {
    if (!log) return;
    StatusNode *cur = log->head;
    while (cur) {
        StatusNode *nxt = cur->next;
        free(cur);
        cur = nxt;
    }
    free(log);
}

int append_status(StatusLog *log,
                  const char *tracking, const char *status,
                  const char *staff_id, const char *staff_name,
                  const char *location,
                  const char *date, const char *time_str) {
    if (!log) return 0;
    StatusNode *node = (StatusNode*)malloc(sizeof(StatusNode));
    if (!node) return 0;
    strncpy(node->tracking_number, tracking,   sizeof(node->tracking_number)-1);
    strncpy(node->status,          status,     sizeof(node->status)-1);
    strncpy(node->staff_id,        staff_id,   sizeof(node->staff_id)-1);
    strncpy(node->staff_name,      staff_name, sizeof(node->staff_name)-1);
    strncpy(node->location,        location,   sizeof(node->location)-1);
    strncpy(node->date,            date,       sizeof(node->date)-1);
    strncpy(node->time_str,        time_str,   sizeof(node->time_str)-1);
    node->next = log->head;
    log->head  = node;
    log->count++;
    return 1;
}

void get_current_datetime_status(char *date, char *time_str) {
    time_t t;
    struct tm *tm_info;
    time(&t);
    tm_info = localtime(&t);
    strftime(date,     20, "%Y-%m-%d", tm_info);
    strftime(time_str, 20, "%H:%M:%S", tm_info);
}

int is_valid_status(const char *status) {
    const char *valid[] = {STATUS_BOOKED, STATUS_INTRANSIT,
                           STATUS_OUT_FOR_DELIVERY, STATUS_DELIVERED};
    int i;
    for (i = 0; i < 4; i++)
        if (strcmp(status, valid[i]) == 0) return 1;
    return 0;
}

int save_status_to_csv(const char *tracking, const char *status,
                       const char *staff_id, const char *staff_name,
                       const char *location) {
    if (!is_valid_status(status)) return 0;

    char date[20], time_str[20];
    get_current_datetime_status(date, time_str);

    StatusLog *log = create_status_log();
    if (!log) return 0;
    append_status(log, tracking, status, staff_id,
                  staff_name, location, date, time_str);

    FILE *check = fopen(STATUS_LOG_FILE, "r");
    int exists  = (check != NULL);
    if (check) fclose(check);

    FILE *f = fopen(STATUS_LOG_FILE, "a");
    if (!f) { free_status_log(log); return 0; }

    if (!exists)
        fprintf(f, "tracking_number,status,staff_id,staff_name,location,date,time\n");

    StatusNode *cur = log->head;
    while (cur) {
        fprintf(f, "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n",
                cur->tracking_number, cur->status,
                cur->staff_id, cur->staff_name,
                cur->location, cur->date, cur->time_str);
        cur = cur->next;
    }
    fclose(f);
    free_status_log(log);
    return 1;
}