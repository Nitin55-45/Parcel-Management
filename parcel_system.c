#define _CRT_SECURE_NO_WARNINGS
/* parcel_system.c — Hub-Centric Router with Unified Parcel BST, Hub BST, and Phone BST. */
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

/* ══════════════════════════════════════════════════════════════
   BUILD ALL HUBS — RAM-Sort Optimization.
   Instead of re-reading status_log.csv, we traverse the enriched
   Parcel BST, gather active parcels into a pointer array, sort
   them chronologically with qsort, then populate the Hub BST.
   This eliminates redundant file I/O entirely.
   ══════════════════════════════════════════════════════════════ */

/* Helper: count active (non-delivered, has_status) nodes in the BST */
static int count_active(ParcelNode *n) {
    if (!n) return 0;
    int c = count_active(n->left) + count_active(n->right);
    if (n->has_status && strcasecmp(n->status, "Delivered") != 0) c++;
    return c;
}

/* Helper: gather pointers to active nodes into the array */
static void gather_active(ParcelNode *n, ParcelNode **arr, int *idx) {
    if (!n) return;
    gather_active(n->left, arr, idx);
    if (n->has_status && strcasecmp(n->status, "Delivered") != 0)
        arr[(*idx)++] = n;
    gather_active(n->right, arr, idx);
}

/* Comparator for qsort: sort by status_date then status_time (chronological) */
static int cmp_by_datetime(const void *a, const void *b) {
    const ParcelNode *pa = *(const ParcelNode **)a;
    const ParcelNode *pb = *(const ParcelNode **)b;
    int d = strcmp(pa->status_date, pb->status_date);
    return d ? d : strcmp(pa->status_time, pb->status_time);
}

static HubNode* build_all_hubs(ParcelNode **out_pr) {
    ParcelNode *pr = load_enriched_bst("parcels.csv", "status_log.csv");
    *out_pr = pr;
    if (!pr) return NULL;

    /* Phase 1: Count active parcels */
    int total = count_active(pr);
    if (total == 0) return NULL;

    /* Phase 2: Gather pointers into array */
    ParcelNode **arr = malloc(total * sizeof(ParcelNode*));
    if (!arr) return NULL;
    int idx = 0;
    gather_active(pr, arr, &idx);

    /* Phase 3: RAM-Sort — QuickSort by date+time (chronological order) */
    qsort(arr, idx, sizeof(ParcelNode*), cmp_by_datetime);

    /* Phase 4: Build Hub BST from the sorted array */
    HubNode *hub_root = NULL;
    for (int i = 0; i < idx; i++) {
        ParcelNode *pn = arr[i];

        /* Determine target queue */
        int q_type = -1;
        if (!strcmp(pn->status, "Booked")) q_type = 0;
        else if (!strcmp(pn->status, "InTransit")) q_type = 1;
        else if (!strcmp(pn->status, "Out for Delivery")) q_type = 2;

        if (q_type >= 0) {
            hub_root = hub_bst_insert(hub_root, pn->location);
            HubNode *h = hub_bst_search(hub_root, pn->location);
            if (h) {
                Queue *q = (q_type == 0) ? &h->booked_q : (q_type == 1) ? &h->intransit_q : &h->out_for_delivery_q;
                if (!strncmp(pn->staff_name, "BACK|", 5) || !strcmp(pn->staff_name, "Admin_Override_Back"))
                    enqueue_front(q, pn->data.tracking_number);
                else
                    enqueue(q, pn->data.tracking_number, pn->data.priority);
            }
        }
    }

    free(arr);  /* Free the pointer array (NOT the nodes — those live in the BST) */
    return hub_root;
}

/* ── Build Phone Index ── */
static PhoneNode* build_phone_index(void) {
    FILE *f = fopen("parcels.csv", "r"); if (!f) return NULL;
    PhoneNode *root = NULL; char line[2048];
    fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\r') continue;
        char d[15][256]; int pos = 0;
        for (int i = 0; i < 15; i++) pos = parse_csv_field(line, pos, d[i], 256);
        if (d[2][0]) root = phone_bst_insert(root, d[2], d[0]);
        if (d[6][0]) root = phone_bst_insert(root, d[6], d[0]);
    }
    fclose(f); return root;
}

/* ══════════════════════════════════════════════════════════════
   COMMAND HANDLERS
   ══════════════════════════════════════════════════════════════ */

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
    ParcelNode *pr;
    HubNode *root = build_all_hubs(&pr);
    HubNode *h = hub_bst_search(root, hub);
    if (h) {
        printf("BOOKED:"); for (QueueNode *n = h->booked_q.front; n; n = n->next) printf("%s,", n->tracking_number);
        printf("\nINTRANSIT:"); for (QueueNode *n = h->intransit_q.front; n; n = n->next) printf("%s,", n->tracking_number);
        printf("\nOUTDELIVERY:"); for (QueueNode *n = h->out_for_delivery_q.front; n; n = n->next) printf("%s,", n->tracking_number);
        printf("\n");
    } else {
        printf("BOOKED:\nINTRANSIT:\nOUTDELIVERY:\n");
    }
    hub_bst_free(root); free_bst(pr);
    return 0;
}

static int cmd_process(const char *hub, const char *from_q, const char *aid) {
    ParcelNode *pr;
    HubNode *root = build_all_hubs(&pr);
    HubNode *h = hub_bst_search(root, hub);
    if (!h) { printf("EMPTY\n"); hub_bst_free(root); free_bst(pr); return 0; }

    Queue *src = !strcmp(from_q, "booked") ? &h->booked_q : (!strcmp(from_q, "intransit") ? &h->intransit_q : (!strcmp(from_q, "outdelivery") ? &h->out_for_delivery_q : NULL));
    const char *tgt = !strcmp(from_q, "booked") ? STATUS_INTRANSIT : (!strcmp(from_q, "intransit") ? STATUS_OUT_FOR_DELIVERY : STATUS_DELIVERED);
    if (!src) { hub_bst_free(root); free_bst(pr); return 1; }

    char *trk = dequeue(src);
    if (!trk) { printf("EMPTY\n"); hub_bst_free(root); free_bst(pr); return 0; }

    char loc[50]; strncpy(loc, hub, 49); loc[49] = '\0';
    if (!strcmp(from_q, "booked")) {
        ParcelNode *p = search_bst(pr, trk);
        if (p) {
            strncpy(loc, p->data.receiver_city, 49);
            if (strcasecmp(p->data.sender_city, p->data.receiver_city) == 0) tgt = STATUS_OUT_FOR_DELIVERY;
        }
    }
    save_status_to_csv(trk, tgt, aid, "Admin_Process", loc);
    printf("PROCESSED|%s|%s|%s\n", trk, tgt, loc); free(trk);
    hub_bst_free(root); free_bst(pr);
    return 0;
}

static int cmd_delivery(const char *hub) {
    ParcelNode *pr;
    HubNode *root = build_all_hubs(&pr);
    HubNode *h = hub_bst_search(root, hub);
    if (h) {
        for (QueueNode *qn = h->out_for_delivery_q.front; qn; qn = qn->next) {
            ParcelNode *p = search_bst(pr, qn->tracking_number);
            if (p) {
                const char *pri = p->data.priority ? "Express" : "Standard";
                printf("PARCEL_START\n%s|%s|%s|%s|%s|%s|%s|%s|%s|%.2f|%s|%s|Out for Delivery|%s\nPARCEL_END\n",
                    p->data.tracking_number, p->data.receiver_name, p->data.receiver_contact,
                    p->data.receiver_address, p->data.receiver_city, p->data.sender_name,
                    p->data.sender_contact, p->data.sender_address, p->data.sender_city,
                    p->data.weight, p->data.parcel_type, p->data.special_instructions, pri);
            }
        }
    }
    hub_bst_free(root); free_bst(pr);
    return 0;
}

static int cmd_myparcels(const char *phone) {
    PhoneNode *phone_root = build_phone_index();
    ParcelNode *pr = load_enriched_bst("parcels.csv", "status_log.csv");

    PhoneNode *pn = phone_bst_search(phone_root, phone);
    if (pn) {
        for (TrackingLink *tl = pn->parcels; tl; tl = tl->next) {
            ParcelNode *p = search_bst(pr, tl->tracking_number);
            if (!p) continue;
            const char *st = p->has_status ? p->status : "Booked";
            const char *pri = p->data.priority ? "Express" : "Standard";
            if (!strcasecmp(p->data.sender_contact, phone))
                printf("SENT_START\n%s|%s|%s|%s|%s|%s|%s|%s|%s|%.2f|%s|%s|%s|%s|%s|%s\nSENT_END\n",
                    p->data.tracking_number, p->data.sender_name, p->data.sender_contact,
                    p->data.sender_address, p->data.sender_city, p->data.receiver_name,
                    p->data.receiver_contact, p->data.receiver_address, p->data.receiver_city,
                    p->data.weight, p->data.parcel_type, p->data.special_instructions,
                    st, p->has_status?p->status_date:"", p->has_status?p->status_time:"", pri);
            if (!strcasecmp(p->data.receiver_contact, phone))
                printf("RECV_START\n%s|%s|%s|%s|%s|%s|%s|%s|%s|%.2f|%s|%s|%s|%s|%s|%s\nRECV_END\n",
                    p->data.tracking_number, p->data.sender_name, p->data.sender_contact,
                    p->data.sender_address, p->data.sender_city, p->data.receiver_name,
                    p->data.receiver_contact, p->data.receiver_address, p->data.receiver_city,
                    p->data.weight, p->data.parcel_type, p->data.special_instructions,
                    st, p->has_status?p->status_date:"", p->has_status?p->status_time:"", pri);
        }
    }
    phone_bst_free(phone_root); free_bst(pr);
    return 0;
}

static int cmd_notifications(const char *phone) {
    PhoneNode *phone_root = build_phone_index();
    PhoneNode *pn = phone_bst_search(phone_root, phone);
    if (!pn) { phone_bst_free(phone_root); return 0; }

    FILE *f = fopen("status_log.csv", "r"); if (!f) { phone_bst_free(phone_root); return 0; }
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

        int found = 0;
        for (TrackingLink *tl = pn->parcels; tl; tl = tl->next) {
            if (!strcmp(tl->tracking_number, trk)) { found = 1; break; }
        }
        if (!found) continue;

        ParcelNode *parcel = search_bst(pr, trk);
        if (!parcel) continue;

        int is_s = !strcasecmp(parcel->data.sender_contact, phone);
        char msg_st[100];
        if (strstr(sn, "BACK|")) sprintf(msg_st, "%s (RETURNED)", st);
        else strcpy(msg_st, st);
        printf("NOTIF_START\n%s|%s|%s|%s|%s|%s\nNOTIF_END\n", trk, msg_st, d, t, is_s ? "Sender" : "Receiver", loc);
        cnt++;
    }
    free_bst(pr); phone_bst_free(phone_root);
    return 0;
}

/* ── inorder_print: Used for parcellog and delivered_today ── */
static void inorder_print(ParcelNode *n, const char *hub, int mode, const char *filter_sid) {
    if (!n) return;
    inorder_print(n->left, hub, mode, filter_sid);
    const char *st = n->has_status ? n->status : "Booked";
    const char *pr = n->data.priority ? "Express" : "Standard";
    if (mode == 0) { /* Full parcel log */
        printf("PARCEL_START\n%s|%s|%s|%s|%s|%s|%s|%s|%s|%.2f|%s|%s|%s|%s|%s|%s|%s|%s|%s\nPARCEL_END\n",
            n->data.tracking_number, n->data.sender_name, n->data.sender_contact, n->data.sender_address, n->data.sender_city,
            n->data.receiver_name, n->data.receiver_contact, n->data.receiver_address, n->data.receiver_city,
            n->data.weight, n->data.parcel_type, n->data.special_instructions, n->data.date, n->data.time_str,
            st, n->has_status?n->status_date:"-", n->has_status?n->status_time:"-", pr, n->has_status?n->staff_name:"-");
    } else if (mode == 3) { /* Delivered today in hub */
        if (n->has_status && !strcasecmp(st, "Delivered") && !strcasecmp(n->location, hub)) {
            if (filter_sid && strlen(filter_sid) > 0 && strcmp(n->staff_id, filter_sid)) {
                /* Skip if filtering by staff and sid doesn't match */
            } else {
                char d[20], t[20]; get_current_datetime(d, t);
                if (!strcmp(n->status_date, d))
                    printf("DELIV_START\n%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\nDELIV_END\n",
                        n->data.tracking_number, n->data.sender_name, n->data.sender_contact, n->data.sender_city,
                        n->data.receiver_name, n->data.receiver_contact, n->data.receiver_city,
                        n->status_date, n->status_time, n->data.parcel_type, pr);
            }
        }
    }
    inorder_print(n->right, hub, mode, filter_sid);
}

static int run_inorder(const char *hub, int mode, const char *sid) {
    ParcelNode *pr = load_enriched_bst("parcels.csv", "status_log.csv");
    if (!pr) { printf("NO_PARCELS\n"); return 0; }
    inorder_print(pr, hub, mode, sid);
    free_bst(pr); return 0;
}

/* ══════════════════════════════════════════════════════════════
   MAIN — Command Router
   ══════════════════════════════════════════════════════════════ */
int main(int argc, char *argv[]) {
    if (argc < 2) return 1;
    if (!strcmp(argv[1], "book")) return cmd_book();
    if (!strcmp(argv[1], "hubs")) return cmd_hubs();
    if (!strcmp(argv[1], "queues") && argc > 2) return cmd_queues(argv[2]);
    if (!strcmp(argv[1], "process") && argc > 4) return cmd_process(argv[2], argv[3], argv[4]);
    if (!strcmp(argv[1], "parcellog")) return run_inorder(NULL, 0, NULL);
    if (!strcmp(argv[1], "myparcels") && argc > 2) return cmd_myparcels(argv[2]);
    if (!strcmp(argv[1], "delivery") && argc > 2) return cmd_delivery(argv[2]);
    if (!strcmp(argv[1], "delivered_today") && argc > 2) return run_inorder(argv[2], 3, (argc > 3 ? argv[3] : NULL));
    if (!strcmp(argv[1], "notifs") && argc > 2) return cmd_notifications(argv[2]);
    if (!strcmp(argv[1], "update") && argc > 2) {
        if (!strcmp(argv[2], "deliver") && argc == 6) printf(save_status_to_csv(argv[3], "Delivered", argv[4], "Driver", argv[5]) ? "STATUS_OK\n":"ERROR\n");
        if (!strcmp(argv[2], "override") && argc == 8) printf(save_status_to_csv(argv[3], argv[4], argv[5], argv[7], argv[6]) ? "STATUS_OK\n":"ERROR\n");
        return 0;
    }
    if (!strcmp(argv[1], "search") && argc > 3) {
        if (!strcmp(argv[2], "tracking")) {
            ParcelNode *root = load_enriched_bst("parcels.csv", "status_log.csv");
            ParcelNode *r = search_bst(root, argv[3]);
            if (r) {
                printf("FOUND\n%s|%s|%s|%s|%s|%s|%s|%s|%s|%.2f|%s|%s|%s|%s|%s\n",
                    r->data.tracking_number, r->data.sender_name, r->data.sender_contact,
                    r->data.sender_address, r->data.sender_city, r->data.receiver_name,
                    r->data.receiver_contact, r->data.receiver_address, r->data.receiver_city,
                    r->data.weight, r->data.parcel_type, r->data.special_instructions,
                    r->has_status?r->status:"Booked", r->has_status?r->status_date:r->data.date,
                    r->has_status?r->status_time:r->data.time_str);
            } else printf("NOT_FOUND\n");
            free_bst(root); return 0;
        }
        if (!strcmp(argv[2], "location")) { search_by_hub(argv[3]); return 0; }
        if (!strcmp(argv[2], "date") && argc > 4) { search_by_date("parcels.csv", argv[3], argv[4]); return 0; }
    }
    return 1;
}
