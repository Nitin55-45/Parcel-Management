/* status_update.c — Validate and save status to status_log.csv */
#include <stdio.h>
#include <string.h>
#include "status_update.h"
#include "storage.h"
#define STATUS_LOG_FILE "status_log.csv"

int is_valid_status(const char *s) {
    return strcmp(s, STATUS_BOOKED) == 0 || strcmp(s, STATUS_INTRANSIT) == 0 ||
           strcmp(s, STATUS_OUT_FOR_DELIVERY) == 0 || strcmp(s, STATUS_DELIVERED) == 0;
}

int save_status_to_csv(const char *trk, const char *status,
                       const char *sid, const char *sname, const char *loc) {
    if (!is_valid_status(status)) return 0;
    char d[20], t[20]; get_current_datetime(d, t);
    FILE *chk = fopen(STATUS_LOG_FILE, "r"); int hdr = (chk == NULL); if (chk) fclose(chk);
    FILE *f = fopen(STATUS_LOG_FILE, "a"); if (!f) return 0;
    if (hdr) fprintf(f, "tracking_number,status,staff_id,staff_name,location,date,time\n");
    fprintf(f, "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n", trk, status, sid, sname, loc, d, t);
    fclose(f); return 1;
}