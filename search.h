#ifndef SEARCH_H
#define SEARCH_H

#include "storage.h"

// Define the Node for the Binary Search Tree
typedef struct ParcelNode {
    Parcel data;
    struct ParcelNode *left;
    struct ParcelNode *right;
} ParcelNode;

// BST Core Functions
ParcelNode* create_node(Parcel *parcel_data);
ParcelNode* insert_bst(ParcelNode *root, ParcelNode *new_node);
ParcelNode* search_bst(ParcelNode *root, const char *tracking_number);
void free_bst(ParcelNode *root);

// Master Load Function
ParcelNode* load_parcels_to_bst(const char *filename);
// --- New Hub Extraction BST ---
typedef struct CityNode {
    char city_name[50];
    struct CityNode *left;
    struct CityNode *right;
} CityNode;

CityNode* insert_city(CityNode *root, const char *city);
void print_cities_inorder(CityNode *root);

#endif // SEARCH_H