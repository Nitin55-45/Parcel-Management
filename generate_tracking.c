/* generate_tracking.c — Generate unique tracking number: TRK-YYYYMMDD-NNNN-XXXX */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tracking.h"

void generate_tracking_number(char *out) {
    time_t t = time(NULL); struct tm *tm = localtime(&t);
    char today[10]; strftime(today, sizeof(today), "%Y%m%d", tm);
    char last[10] = ""; int seq = 0;
    FILE *f = fopen(COUNTER_FILE, "r");
    if (f) { fscanf(f, "%s %d", last, &seq); fclose(f); }
    seq = (strcmp(last, today) == 0) ? seq + 1 : 1;
    f = fopen(COUNTER_FILE, "w");
    if (f) { fprintf(f, "%s %d", today, seq); fclose(f); }
    srand((unsigned)time(NULL) + seq);
    const char cs[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char sf[5]; for (int i = 0; i < 4; i++) sf[i] = cs[rand() % 36]; sf[4] = '\0';
    sprintf(out, "TRK-%s-%04d-%s", today, seq, sf);
}