#define _CRT_SECURE_NO_WARNINGS
/* search.c — Unified Parcel BST (Identity + Status merged) + date search */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "search.h"

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

/* ── Parcel BST ── */
ParcelNode* insert_bst(ParcelNode *root, ParcelNode *node) {
    if (!root) return node;
    int c = strcmp(node->data.tracking_number, root->data.tracking_number);
    if (c < 0) root->left = insert_bst(root->left, node);
    else if (c > 0) root->right = insert_bst(root->right, node);
    return root;
}

ParcelNode* search_bst(ParcelNode *root, const char *tracking) {
    if (!root) return NULL;
    int c = strcasecmp(tracking, root->data.tracking_number);
    return (c == 0) ? root : search_bst(c < 0 ? root->left : root->right, tracking);
}

void free_bst(ParcelNode *root) {
    if (!root) return;
    free_bst(root->left); free_bst(root->right); free(root);
}

/* CSV column order: trk,sname,scon,saddr,scity,rname,rcon,raddr,rcity,weight,type,instr,priority,date,time */
ParcelNode* load_parcels_to_bst(const char *fn) {
    FILE *f = fopen(fn, "r"); if (!f) return NULL;
    ParcelNode *root = NULL; char line[2048];
    fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\r') continue;
        char d[15][256]; int pos = 0;
        for (int i = 0; i < 15; i++) pos = parse_csv_field(line, pos, d[i], 256);

        ParcelNode *n = calloc(1, sizeof(ParcelNode));
        if (n) {
            strncpy(n->data.tracking_number, d[0], 49); strncpy(n->data.sender_name, d[1], 99);
            strncpy(n->data.sender_contact, d[2], 19); strncpy(n->data.sender_address, d[3], 199);
            strncpy(n->data.sender_city, d[4], 49); strncpy(n->data.receiver_name, d[5], 99);
            strncpy(n->data.receiver_contact, d[6], 19); strncpy(n->data.receiver_address, d[7], 199);
            strncpy(n->data.receiver_city, d[8], 49); n->data.weight = atof(d[9]);
            strncpy(n->data.parcel_type, d[10], 49); strncpy(n->data.special_instructions, d[11], 199);
            n->data.priority = atoi(d[12]);
            strncpy(n->data.date, d[13], 19); strncpy(n->data.time_str, d[14], 19);
            n->has_status = 0;
            n->left = n->right = NULL;
            root = insert_bst(root, n);
        }
    }
    fclose(f); return root;
}

/* ── Status Enrichment ──
   Reads status_log.csv and updates ParcelNodes in-place.
   Because the CSV is chronological, the last row for each tracking number
   becomes the "latest" status — same overwrite logic as the old Status BST. */
void enrich_bst_with_status(ParcelNode *root, const char *fn) {
    if (!root) return;
    FILE *f = fopen(fn, "r"); if (!f) return;
    char line[512];
    fgets(line, sizeof(line), f); /* skip header */
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\r') continue;
        char trk[50], st[50], sid[50], sn[100], loc[100], dt[20], tm[20]; int p = 0;
        p = parse_csv_field(line, p, trk, 50); p = parse_csv_field(line, p, st, 50);
        p = parse_csv_field(line, p, sid, 50); p = parse_csv_field(line, p, sn, 100);
        p = parse_csv_field(line, p, loc, 100); p = parse_csv_field(line, p, dt, 20);
        parse_csv_field(line, p, tm, 20);

        ParcelNode *node = search_bst(root, trk);
        if (node) {
            strncpy(node->status, st, 29); node->status[29] = '\0';
            strncpy(node->status_date, dt, 19); node->status_date[19] = '\0';
            strncpy(node->status_time, tm, 19); node->status_time[19] = '\0';
            strncpy(node->location, loc, 99); node->location[99] = '\0';
            strncpy(node->staff_id, sid, 49); node->staff_id[49] = '\0';
            strncpy(node->staff_name, sn, 99); node->staff_name[99] = '\0';
            node->has_status = 1;
        }
    }
    fclose(f);
}

/* Combined load + enrich */
ParcelNode* load_enriched_bst(const char *parcels_file, const char *status_file) {
    ParcelNode *root = load_parcels_to_bst(parcels_file);
    enrich_bst_with_status(root, status_file);
    return root;
}

/* ── Date search (Linear Scan) ── */
void search_by_date(const char *fn, const char *from, const char *to) {
    FILE *f = fopen(fn, "r"); if (!f) return;
    ParcelNode *pr = load_enriched_bst("parcels.csv", "status_log.csv");
    char line[2048]; fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\r') continue;
        char d[15][256]; int pos = 0;
        for (int i = 0; i < 15; i++) pos = parse_csv_field(line, pos, d[i], 256);

        if (strcmp(d[12], from) >= 0 && strcmp(d[12], to) <= 0) {
            ParcelNode *p = search_bst(pr, d[0]);
            const char *st = (p && p->has_status) ? p->status : "Booked";
            const char *s_dt = (p && p->has_status) ? p->status_date : d[12];
            const char *s_tm = (p && p->has_status) ? p->status_time : d[13];
            printf("RESULT_START\nTracking No: %s\nSender: %s | %s | %s | %s\nReceiver: %s | %s | %s | %s\nWeight: %s\nParcel Type: %s\nInstructions: %s\nPriority: %s\nDate: %s\nTime: %s\nStatus: %s\nRESULT_END\n",
                   d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11],
                   atoi(d[14]) ? "Express" : "Standard", s_dt, s_tm, st);
        }
    }
    free_bst(pr); fclose(f);
}