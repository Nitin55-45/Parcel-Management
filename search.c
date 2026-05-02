#define _CRT_SECURE_NO_WARNINGS
/* search.c — BST for parcel lookup + status lookup + date search */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "search.h"

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

/* --- Parcel BST --- */
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

        ParcelNode *n = malloc(sizeof(ParcelNode));
        if (n) {
            strncpy(n->data.tracking_number, d[0], 49); strncpy(n->data.sender_name, d[1], 99);
            strncpy(n->data.sender_contact, d[2], 19); strncpy(n->data.sender_address, d[3], 199);
            strncpy(n->data.sender_city, d[4], 49); strncpy(n->data.receiver_name, d[5], 99);
            strncpy(n->data.receiver_contact, d[6], 19); strncpy(n->data.receiver_address, d[7], 199);
            strncpy(n->data.receiver_city, d[8], 49); n->data.weight = atof(d[9]);
            strncpy(n->data.parcel_type, d[10], 49); strncpy(n->data.special_instructions, d[11], 199);
            n->data.priority = atoi(d[12]);
            strncpy(n->data.date, d[13], 19); strncpy(n->data.time_str, d[14], 19);
            n->left = n->right = NULL;
            root = insert_bst(root, n);
        }
    }
    fclose(f); return root;
}

/* --- Status BST --- */
StatusNode* insert_status_bst(StatusNode *root, StatusNode *node) {
    if (!root) return node;
    int c = strcmp(node->tracking_number, root->tracking_number);
    if (c < 0) root->left = insert_status_bst(root->left, node);
    else if (c > 0) root->right = insert_status_bst(root->right, node);
    else {
        strncpy(root->status, node->status, 29); strncpy(root->date, node->date, 19);
        strncpy(root->time_str, node->time_str, 19); strncpy(root->location, node->location, 99);
        strncpy(root->staff_id, node->staff_id, 49); strncpy(root->staff_name, node->staff_name, 99);
        free(node);
    }
    return root;
}

StatusNode* search_status_bst(StatusNode *root, const char *trk) {
    if (!root) return NULL;
    int c = strcasecmp(trk, root->tracking_number);
    return (c == 0) ? root : search_status_bst(c < 0 ? root->left : root->right, trk);
}

void free_status_bst(StatusNode *root) {
    if (!root) return;
    free_status_bst(root->left); free_status_bst(root->right); free(root);
}

StatusNode* load_status_to_bst(const char *fn) {
    FILE *f = fopen(fn, "r"); if (!f) return NULL;
    StatusNode *root = NULL; char line[512];
    fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\r') continue;
        char trk[50], st[50], sid[50], sn[100], loc[100], dt[20], tm[20]; int p = 0;
        p = parse_csv_field(line, p, trk, 50); p = parse_csv_field(line, p, st, 50);
        p = parse_csv_field(line, p, sid, 50); p = parse_csv_field(line, p, sn, 100);
        p = parse_csv_field(line, p, loc, 100); p = parse_csv_field(line, p, dt, 20);
        parse_csv_field(line, p, tm, 20);

        StatusNode *n = calloc(1, sizeof(StatusNode));
        strncpy(n->tracking_number, trk, 49); strncpy(n->status, st, 29);
        strncpy(n->date, dt, 19); strncpy(n->time_str, tm, 19); strncpy(n->location, loc, 99);
        strncpy(n->staff_id, sid, 49); strncpy(n->staff_name, sn, 99);
        root = insert_status_bst(root, n);
    }
    fclose(f); return root;
}

/* --- Date search (Linear Scan) --- */
void search_by_date(const char *fn, const char *from, const char *to) {
    FILE *f = fopen(fn, "r"); if (!f) return;
    StatusNode *sr = load_status_to_bst("status_log.csv");
    char line[2048]; fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\r') continue;
        char d[15][256]; int pos = 0;
        for (int i = 0; i < 15; i++) pos = parse_csv_field(line, pos, d[i], 256);

        if (strcmp(d[13], from) >= 0 && strcmp(d[13], to) <= 0) {
            StatusNode *s = search_status_bst(sr, d[0]);
            printf("RESULT_START\nTracking No: %s\nSender: %s | %s | %s | %s\nReceiver: %s | %s | %s | %s\nWeight: %s\nType: %s\nInstructions: %s\nPriority: %s\nDate: %s\nTime: %s\nStatus: %s\nRESULT_END\n",
                   d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11],
                   atoi(d[12]) ? "Express" : "Standard",
                   s ? s->date : d[13], s ? s->time_str : d[14], s ? s->status : "Booked");
        }
    }
    free_status_bst(sr); fclose(f);
}