#define _CRT_SECURE_NO_WARNINGS
/* storage.c — CSV read/write for parcels */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "storage.h"
#define PARCELS_FILE "parcels.csv"

void get_current_datetime(char *date, char *ts) {
    time_t t = time(NULL); struct tm *tm = localtime(&t);
    strftime(date, 20, "%Y-%m-%d", tm); strftime(ts, 20, "%H:%M:%S", tm);
}

int parse_csv_field(const char *line, int pos, char *out, int sz) {
    int i = 0; out[0] = '\0';
    if (line[pos] == '"') {
        pos++;
        while (line[pos] && i < sz-1) {
            if (line[pos] == '"') { if (line[pos+1] == '"') { out[i++] = '"'; pos += 2; } else { pos++; break; } }
            else out[i++] = line[pos++];
        }
    } else {
        while (line[pos] && line[pos] != ',' && line[pos] != '\n' && line[pos] != '\r' && i < sz-1)
            out[i++] = line[pos++];
    }
    out[i] = '\0';
    while (i > 0 && isspace((unsigned char)out[i-1])) out[--i] = '\0';
    int start = 0; while (isspace((unsigned char)out[start])) start++;
    if (start > 0) memmove(out, out + start, i - start + 1);
    if (line[pos] == ',') pos++;
    return pos;
}

void save_to_csv(Parcel *p) {
    char date[20], ts[20]; get_current_datetime(date, ts);
    FILE *chk = fopen(PARCELS_FILE, "r"); int hdr = (chk == NULL); if (chk) fclose(chk);
    FILE *f = fopen(PARCELS_FILE, "a");
    if (!f) { perror("[ERROR] parcels.csv"); return; }
    if (hdr) fprintf(f, "tracking_number,sender_name,sender_contact,sender_address,"
        "sender_city,receiver_name,receiver_contact,receiver_address,"
        "receiver_city,weight_kg,parcel_type,special_instructions,priority,date,time\n");
    char sp[200]; strncpy(sp, p->special_instructions, 199);
    size_t l = strlen(sp); if (l > 0 && sp[l-1] == '\n') sp[l-1] = '\0';
    if (!strlen(sp)) strcpy(sp, "None");
    fprintf(f, "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",%.2f,\"%s\",\"%s\",%d,\"%s\",\"%s\"\n",
        p->tracking_number, p->sender_name, p->sender_contact, p->sender_address, p->sender_city,
        p->receiver_name, p->receiver_contact, p->receiver_address, p->receiver_city,
        p->weight, p->parcel_type, sp, p->priority, date, ts);
    fclose(f);
}