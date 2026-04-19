#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "tracking.h"
int read_counter(char *last_date) {
    FILE *f = fopen(COUNTER_FILE, "r");
    if (!f) { strcpy(last_date, ""); return 0; }
    int seq = 0;
    fscanf(f, "%s %d", last_date, &seq);
    fclose(f);
    return seq;
}
 
void write_counter(const char *date, int seq) {
    FILE *f = fopen(COUNTER_FILE, "w");
    if (!f) return;
    fprintf(f, "%s %d", date, seq);
    fclose(f);
}
 
void make_suffix(char *suffix, int seed_extra) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    srand((unsigned int)time(NULL) + seed_extra);
    for (int i = 0; i < 4; i++)
        suffix[i] = charset[rand() % 36];
    suffix[4] = '\0';
}
 
void generate_tracking_number(char *out) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char today[10];
    strftime(today, sizeof(today), "%Y%m%d", tm_info);
 
    char last_date[10] = "";
    int seq = read_counter(last_date);
 
    if (strcmp(last_date, today) == 0)
        seq += 1;
    else
        seq = 1;
 
    write_counter(today, seq);
 
    char suffix[5];
    make_suffix(suffix, seq);
 
    sprintf(out, "TRK-%s-%04d-%s", today, seq, suffix);
}
 
