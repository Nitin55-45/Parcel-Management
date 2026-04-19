#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "search.h"

#define MAX_FIELD 256

// Safely parse CSV to handle quotes
static int parse_field(const char *line, int pos, char *out, int out_sz) {
    int i = 0;
    out[0] = '\0';
    if (line[pos] == '"') {
        pos++; 
        while (line[pos] && i < out_sz - 1) {
            if (line[pos] == '"') {
                if (line[pos + 1] == '"') { 
                    out[i++] = '"';
                    pos += 2;
                } else {
                    pos++; 
                    break;
                }
            } else {
                out[i++] = line[pos++];
            }
        }
    } else {
        while (line[pos] && line[pos] != ',' && line[pos] != '\n' &&
               line[pos] != '\r' && i < out_sz - 1) {
            out[i++] = line[pos++];
        }
    }
    out[i] = '\0';
    if (line[pos] == ',') pos++; 
    return pos;
}

ParcelNode* create_node(Parcel *parcel_data) {
    ParcelNode *new_node = (ParcelNode*)malloc(sizeof(ParcelNode));
    if (new_node != NULL) {
        new_node->data = *parcel_data;
        new_node->left = NULL;
        new_node->right = NULL;
    }
    return new_node;
}

ParcelNode* insert_bst(ParcelNode *root, ParcelNode *new_node) {
    if (root == NULL) return new_node;
    int cmp = strcmp(new_node->data.tracking_number, root->data.tracking_number);
    if (cmp < 0) root->left = insert_bst(root->left, new_node);
    else if (cmp > 0) root->right = insert_bst(root->right, new_node);
    return root;
}

ParcelNode* search_bst(ParcelNode *root, const char *tracking_number) {
    if (root == NULL || strcasecmp(root->data.tracking_number, tracking_number) == 0) {
        return root;
    }
    if (strcasecmp(tracking_number, root->data.tracking_number) > 0) {
        return search_bst(root->right, tracking_number);
    }
    return search_bst(root->left, tracking_number);
}

void free_bst(ParcelNode *root) {
    if (root != NULL) {
        free_bst(root->left);
        free_bst(root->right);
        free(root);
    }
}

ParcelNode* load_parcels_to_bst(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return NULL;

    ParcelNode *root = NULL;
    char line[2048]; 
    fgets(line, sizeof(line), file); // Skip header

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '\n' || line[0] == '\r') continue;
        char f[14][MAX_FIELD];
        int pos = 0;
        for (int i = 0; i < 14; i++) pos = parse_field(line, pos, f[i], MAX_FIELD);

        Parcel p;
        strcpy(p.tracking_number, f[0]);
        strcpy(p.sender_name, f[1]);
        strcpy(p.sender_contact, f[2]);
        strcpy(p.sender_address, f[3]);
        strcpy(p.sender_city, f[4]);
        strcpy(p.receiver_name, f[5]);
        strcpy(p.receiver_contact, f[6]);
        strcpy(p.receiver_address, f[7]);
        strcpy(p.receiver_city, f[8]);
        p.weight = atof(f[9]);
        strcpy(p.parcel_type, f[10]);
        strcpy(p.special_instructions, f[11]);

        ParcelNode *new_node = create_node(&p);
        root = insert_bst(root, new_node);
    }
    
    fclose(file);
    return root;
}

// --- New Hub Extraction BST Implementation ---
// These are now correctly placed outside of load_parcels_to_bst

CityNode* insert_city(CityNode *root, const char *city) {
    if (root == NULL) {
        CityNode *new_node = (CityNode*)malloc(sizeof(CityNode));
        strncpy(new_node->city_name, city, sizeof(new_node->city_name) - 1);
        new_node->city_name[sizeof(new_node->city_name) - 1] = '\0';
        new_node->left = new_node->right = NULL;
        return new_node;
    }
    int cmp = strcasecmp(city, root->city_name);
    if (cmp < 0) {
        root->left = insert_city(root->left, city);
    } else if (cmp > 0) {
        root->right = insert_city(root->right, city);
    }
    // If cmp == 0, duplicate exists, do nothing (automatic deduplication)
    return root;
}

void print_cities_inorder(CityNode *root) {
    if (root != NULL) {
        print_cities_inorder(root->left);
        printf("%s\n", root->city_name);
        print_cities_inorder(root->right);
    }
}