#define _CRT_SECURE_NO_WARNINGS
/* parcel_system.c — Command router with Priority Queue support. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "details.h"
#include "storage.h"
#include "tracking.h"
#include "hubs.h"
#include "status_update.h"
#include "search.h"
#include "hub_search.h"

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

/* Build hub queues from status_log.csv — priority-aware */
static void build_hub_queues(const char *hub, Queue *bk, Queue *it, Queue *od) {
    init_queue(bk); init_queue(it); init_queue(od);
    StatusNode *sr = load_status_to_bst("status_log.csv");
    ParcelNode *pr = load_parcels_to_bst("parcels.csv");
    FILE *f = fopen("status_log.csv", "r");
    if (!f) { free_status_bst(sr); free_bst(pr); return; }

    char line[512], trk[50], st[50], sid[50], sn[100], loc[100], dt[20], tm[20];
    fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\r') continue;
        int p = 0;
        p = parse_csv_field(line, p, trk, 50); p = parse_csv_field(line, p, st, 50);
        p = parse_csv_field(line, p, sid, 50); p = parse_csv_field(line, p, sn, 100);
        p = parse_csv_field(line, p, loc, 100); p = parse_csv_field(line, p, dt, 20);
        parse_csv_field(line, p, tm, 20);

        StatusNode *latest = search_status_bst(sr, trk);
        if (latest && !strcmp(st, latest->status) && !strcmp(dt, latest->date) && !strcmp(tm, latest->time_str)) {
            if (strcasecmp(loc, hub)) continue;
            Queue *q = !strcmp(st, "Booked") ? bk : (!strcmp(st, "InTransit") ? it : (!strcmp(st, "Out for Delivery") ? od : NULL));
            if (q) {
                ParcelNode *pn = search_bst(pr, trk);
                int prio = pn ? pn->data.priority : 0;
                if (!strncmp(sn, "BACK|", 5) || !strcmp(sn, "Admin_Override_Back")) enqueue_front(q, trk);
                else enqueue(q, trk, prio);
            }
        }
    }
    fclose(f); free_status_bst(sr); free_bst(pr);
}

static int cmd_book() {
    Person s, r; ParcelInfo pi; Parcel p; char trk[30], d[20], t[20];
    input_person(&s, "Sender"); input_person(&r, "Receiver"); input_parcel_info(&pi);
    p.priority = pi.priority;
    generate_tracking_number(trk);
    strcpy(p.tracking_number, trk); strcpy(p.sender_name, s.name); strcpy(p.sender_contact, s.phone);
    strcpy(p.sender_address, s.address); strcpy(p.sender_city, s.city);
    strcpy(p.receiver_name, r.name); strcpy(p.receiver_contact, r.phone);
    strcpy(p.receiver_address, r.address); strcpy(p.receiver_city, r.city);
    strcpy(p.parcel_type, pi.type); p.weight = pi.weight;
    strcpy(p.special_instructions, strlen(pi.specialInstructions) > 1 ? pi.specialInstructions : "None");
    save_to_csv(&p); save_status_to_csv(trk, "Booked", "SYS01", "System Auto", s.city);
    get_current_datetime(d, t);
    printf("Tracking No : %s\nDate : %s %s\n", trk, d, t);
    return 0;
}

static int cmd_hubs() {
    FILE *f = fopen("parcels.csv", "r");
    if (!f) { printf("CITIES:\n"); return 0; }
    char cities[256][50], line[2048]; int count = 0;
    fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f)) {
        char sc[50], rc[50], tmp[256]; int pos = 0;
        for (int i = 0; i < 4; i++) pos = parse_csv_field(line, pos, tmp, 256);
        pos = parse_csv_field(line, pos, sc, 50);
        for (int i = 0; i < 3; i++) pos = parse_csv_field(line, pos, tmp, 256);
        parse_csv_field(line, pos, rc, 50);
        char *both[2] = {sc, rc};
        for (int b = 0; b < 2; b++) {
            if (!both[b][0]) continue;
            both[b][0] = toupper(both[b][0]);
            for (int i = 1; both[b][i]; i++) both[b][i] = tolower(both[b][i]);
            int found = 0;
            for (int i = 0; i < count; i++) if (!strcasecmp(cities[i], both[b])) { found = 1; break; }
            if (!found && count < 256) strncpy(cities[count++], both[b], 49);
        }
    }
    fclose(f); printf("CITIES:");
    for (int i = 0; i < count; i++) printf("%s,", cities[i]);
    printf("\n"); return 0;
}

static int cmd_queues(const char *hub) {
    Queue bk, it, od; build_hub_queues(hub, &bk, &it, &od);
    printf("BOOKED:"); for (QueueNode *n = bk.front; n; n = n->next) printf("%s,", n->tracking_number);
    printf("\nINTRANSIT:"); for (QueueNode *n = it.front; n; n = n->next) printf("%s,", n->tracking_number);
    printf("\nOUTDELIVERY:"); for (QueueNode *n = od.front; n; n = n->next) printf("%s,", n->tracking_number);
    printf("\n"); return 0;
}

static int cmd_process(const char *hub, const char *from_q, const char *aid) {
    Queue bk, it, od; build_hub_queues(hub, &bk, &it, &od);
    Queue *src = !strcmp(from_q, "booked") ? &bk : (!strcmp(from_q, "intransit") ? &it : (!strcmp(from_q, "outdelivery") ? &od : NULL));
    const char *tgt = !strcmp(from_q, "booked") ? STATUS_INTRANSIT : (!strcmp(from_q, "intransit") ? STATUS_OUT_FOR_DELIVERY : STATUS_DELIVERED);
    if (!src) return 1;
    char *trk = dequeue(src);
    if (!trk) { printf("EMPTY\n"); return 0; }
    char loc[50]; strncpy(loc, hub, 49); loc[49] = '\0';
    if (!strcmp(from_q, "booked")) {
        ParcelNode *root = load_parcels_to_bst("parcels.csv");
        ParcelNode *p = search_bst(root, trk);
        if (p) {
            strncpy(loc, p->data.receiver_city, 49);
            if (strcasecmp(p->data.sender_city, p->data.receiver_city) == 0) tgt = STATUS_OUT_FOR_DELIVERY;
        }
        free_bst(root);
    }
    save_status_to_csv(trk, tgt, aid, "Admin_Process", loc);
    printf("PROCESSED|%s|%s|%s\n", trk, tgt, loc); free(trk); return 0;
}

static void inorder_print(ParcelNode *n, StatusNode *sr, const char *ph, const char *hub, int mode, const char *filter_sid) {
    if (!n) return;
    inorder_print(n->left, sr, ph, hub, mode, filter_sid);
    StatusNode *s = search_status_bst(sr, n->data.tracking_number);
    const char *st = s ? s->status : "Booked";
    const char *pr = n->data.priority ? "Express" : "Standard";
    if (mode == 0) { // Log
        printf("PARCEL_START\n%s|%s|%s|%s|%s|%s|%s|%s|%s|%.2f|%s|%s|%s|%s|%s|%s|%s|%s|%s\nPARCEL_END\n",
            n->data.tracking_number, n->data.sender_name, n->data.sender_contact, n->data.sender_address, n->data.sender_city,
            n->data.receiver_name, n->data.receiver_contact, n->data.receiver_address, n->data.receiver_city,
            n->data.weight, n->data.parcel_type, n->data.special_instructions, n->data.date, n->data.time_str, st, s?s->date:"-", s?s->time_str:"-", pr, s?s->staff_name:"-");
    } else if (mode == 1) { // My Parcels
        if (!strcasecmp(n->data.sender_contact, ph)) printf("SENT_START\n%s|%s|%s|%s|%s|%s|%s|%s|%s|%.2f|%s|%s|%s|%s|%s|%s\nSENT_END\n", n->data.tracking_number, n->data.sender_name, n->data.sender_contact, n->data.sender_address, n->data.sender_city, n->data.receiver_name, n->data.receiver_contact, n->data.receiver_address, n->data.receiver_city, n->data.weight, n->data.parcel_type, n->data.special_instructions, st, s?s->date:"", s?s->time_str:"", pr);
        if (!strcasecmp(n->data.receiver_contact, ph)) printf("RECV_START\n%s|%s|%s|%s|%s|%s|%s|%s|%s|%.2f|%s|%s|%s|%s|%s|%s\nRECV_END\n", n->data.tracking_number, n->data.sender_name, n->data.sender_contact, n->data.sender_address, n->data.sender_city, n->data.receiver_name, n->data.receiver_contact, n->data.receiver_address, n->data.receiver_city, n->data.weight, n->data.parcel_type, n->data.special_instructions, st, s?s->date:"", s?s->time_str:"", pr);
    } else if (mode == 2) { // Delivery (Out for Delivery)
        if (s && !strcasecmp(st, "Out for Delivery") && !strcasecmp(s->location, hub))
            printf("PARCEL_START\n%s|%s|%s|%s|%s|%s|%s|%s|%s|%.2f|%s|%s|%s|%s\nPARCEL_END\n", n->data.tracking_number, n->data.receiver_name, n->data.receiver_contact, n->data.receiver_address, n->data.receiver_city, n->data.sender_name, n->data.sender_contact, n->data.sender_address, n->data.sender_city, n->data.weight, n->data.parcel_type, n->data.special_instructions, st, pr);
    } else if (mode == 3) { // Delivered Today in Hub
        if (s && !strcasecmp(st, "Delivered") && !strcasecmp(s->location, hub)) {
            if (filter_sid && strlen(filter_sid) > 0 && strcmp(s->staff_id, filter_sid)) {
                // Skip if filtering and sid doesn't match
            } else {
                char d[20], t[20]; get_current_datetime(d, t);
                if (!strcmp(s->date, d))
                    printf("DELIV_START\n%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\nDELIV_END\n", 
                        n->data.tracking_number, n->data.sender_name, n->data.sender_contact, n->data.sender_city,
                        n->data.receiver_name, n->data.receiver_contact, n->data.receiver_city, s->date, s->time_str, n->data.parcel_type, pr);
            }
        }
    }
    inorder_print(n->right, sr, ph, hub, mode, filter_sid);
}

static int run_inorder(const char *ph, const char *hub, int mode, const char *sid) {
    ParcelNode *pr = load_parcels_to_bst("parcels.csv");
    StatusNode *sr = load_status_to_bst("status_log.csv");
    if (!pr) { printf("NO_PARCELS\n"); return 0; }
    inorder_print(pr, sr, ph, hub, mode, sid);
    free_bst(pr); free_status_bst(sr); return 0;
}

static int cmd_notifications(const char *phone) {
    FILE *f = fopen("status_log.csv", "r"); if (!f) return 0;
    char lines[100][512]; int total = 0, cnt = 0;
    fseek(f, 0, SEEK_END); long sz = ftell(f);
    if (sz > 5120) { fseek(f, -5120, SEEK_END); } else { rewind(f); }
    char tmp[512]; fgets(tmp, 512, f);
    while (total < 100 && fgets(lines[total], 512, f)) total++;
    fclose(f);
    ParcelNode *pr = load_parcels_to_bst("parcels.csv");
    for (int i = total - 1; i >= 0 && cnt < 10; i--) {
        char trk[50], st[50], sid[50], sn[100], loc[100], d[20], t[20]; int p = 0;
        p = parse_csv_field(lines[i], p, trk, 50); p = parse_csv_field(lines[i], p, st, 50);
        p = parse_csv_field(lines[i], p, sid, 50); p = parse_csv_field(lines[i], p, sn, 100);
        p = parse_csv_field(lines[i], p, loc, 100); p = parse_csv_field(lines[i], p, d, 20);
        parse_csv_field(lines[i], p, t, 20);
        ParcelNode *pn = search_bst(pr, trk);
        if (pn) {
            int is_s = !strcasecmp(pn->data.sender_contact, phone), is_r = !strcasecmp(pn->data.receiver_contact, phone);
            if (is_s || is_r) {
                char msg_st[100];
                if (strstr(sn, "BACK|")) sprintf(msg_st, "%s (RETURNED)", st);
                else strcpy(msg_st, st);
                printf("NOTIF_START\n%s|%s|%s|%s|%s|%s\nNOTIF_END\n", trk, msg_st, d, t, is_s ? "Sender" : "Receiver", loc); 
                cnt++; 
            }
        }
    }
    free_bst(pr); return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;
    if (!strcmp(argv[1], "book")) return cmd_book();
    if (!strcmp(argv[1], "hubs")) return cmd_hubs();
    if (!strcmp(argv[1], "queues") && argc > 2) return cmd_queues(argv[2]);
    if (!strcmp(argv[1], "process") && argc > 4) return cmd_process(argv[2], argv[3], argv[4]);
    if (!strcmp(argv[1], "parcellog")) return run_inorder(NULL, NULL, 0, NULL);
    if (!strcmp(argv[1], "myparcels") && argc > 2) return run_inorder(argv[2], NULL, 1, NULL);
    if (!strcmp(argv[1], "delivery") && argc > 2) return run_inorder(NULL, argv[2], 2, NULL);
    if (!strcmp(argv[1], "delivered_today") && argc > 2) return run_inorder(NULL, argv[2], 3, (argc > 3 ? argv[3] : NULL));
    if (!strcmp(argv[1], "notifs") && argc > 2) return cmd_notifications(argv[2]);
    if (!strcmp(argv[1], "update") && argc > 2) {
        if (!strcmp(argv[2], "deliver") && argc == 6) printf(save_status_to_csv(argv[3], "Delivered", argv[4], "Driver", argv[5]) ? "STATUS_OK\n":"ERROR\n");
        if (!strcmp(argv[2], "override") && argc == 8) printf(save_status_to_csv(argv[3], argv[4], argv[5], argv[7], argv[6]) ? "STATUS_OK\n":"ERROR\n");
        return 0;
    }
    if (!strcmp(argv[1], "search") && argc > 3) {
        if (!strcmp(argv[2], "tracking")) {
            ParcelNode *root = load_parcels_to_bst("parcels.csv"), *r = search_bst(root, argv[3]);
            if (r) {
                StatusNode *sr = load_status_to_bst("status_log.csv"), *s = search_status_bst(sr, r->data.tracking_number);
                printf("FOUND\n%s|%s|%s|%s|%s|%s|%s|%s|%s|%.2f|%s|%s|%s|%s|%s\n", r->data.tracking_number, r->data.sender_name, r->data.sender_contact, r->data.sender_address, r->data.sender_city, r->data.receiver_name, r->data.receiver_contact, r->data.receiver_address, r->data.receiver_city, r->data.weight, r->data.parcel_type, r->data.special_instructions, s?s->status:"Booked", s?s->date:r->data.date, s?s->time_str:r->data.time_str);
                free_status_bst(sr);
            } else printf("NOT_FOUND\n");
            free_bst(root); return 0;
        }
        if (!strcmp(argv[2], "location")) { search_by_hub(argv[3]); return 0; }
        if (!strcmp(argv[2], "date") && argc > 4) { search_by_date("parcels.csv", argv[3], argv[4]); return 0; }
    }
    return 1;
}
