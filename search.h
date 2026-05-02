#ifndef SEARCH_H
#define SEARCH_H
#include "storage.h"

typedef struct ParcelNode {
    Parcel data;
    struct ParcelNode *left, *right;
} ParcelNode;

typedef struct StatusNode {
    char tracking_number[50];
    char status[30];
    char date[20];
    char time_str[20];
    char location[100];
    char staff_id[50];
    char staff_name[100];
    struct StatusNode *left, *right;
} StatusNode;

/* Parcel BST */
ParcelNode* insert_bst(ParcelNode *root, ParcelNode *node);
ParcelNode* search_bst(ParcelNode *root, const char *tracking);
void        free_bst(ParcelNode *root);
ParcelNode* load_parcels_to_bst(const char *filename);

/* Status BST */
StatusNode* insert_status_bst(StatusNode *root, StatusNode *node);
StatusNode* search_status_bst(StatusNode *root, const char *tracking);
void        free_status_bst(StatusNode *root);
StatusNode* load_status_to_bst(const char *filename);

/* Date search (linear — unavoidable) */
void search_by_date(const char *filename, const char *from_date, const char *to_date);

#endif