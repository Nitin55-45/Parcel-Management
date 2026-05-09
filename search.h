#ifndef SEARCH_H
#define SEARCH_H
#include "storage.h"

/* Unified ParcelNode — Identity + Status merged into one BST node */
typedef struct ParcelNode {
    Parcel data;
    /* Status fields (merged from former StatusNode) */
    char status[30];
    char status_date[20];
    char status_time[20];
    char location[100];
    char staff_id[50];
    char staff_name[100];
    int has_status;  /* 0 = no status loaded, 1 = enriched */
    struct ParcelNode *left, *right;
} ParcelNode;

/* Parcel BST */
ParcelNode* insert_bst(ParcelNode *root, ParcelNode *node);
ParcelNode* search_bst(ParcelNode *root, const char *tracking);
void        free_bst(ParcelNode *root);
ParcelNode* load_parcels_to_bst(const char *filename);

/* Status enrichment — reads status_log.csv and updates ParcelNodes in-place */
void enrich_bst_with_status(ParcelNode *root, const char *status_filename);

/* Combined load + enrich (convenience) */
ParcelNode* load_enriched_bst(const char *parcels_file, const char *status_file);

/* Date search */
void search_by_date(const char *filename, const char *from_date, const char *to_date);

#endif