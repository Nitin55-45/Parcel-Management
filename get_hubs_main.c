#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "search.h"

#define MAX_FIELD 256

// This is the CRITICAL function that ignores commas inside quotes
static int parse_field(const char *line, int pos, char *out, int out_sz) {
    int i = 0;
    out[0] = '\0';
    if (line[pos] == '"') {
        pos++; // Skip opening quote
        while (line[pos] && i < out_sz - 1) {
            if (line[pos] == '"') {
                if (line[pos + 1] == '"') { 
                    out[i++] = '"'; pos += 2; // Handle escaped quotes
                } else {
                    pos++; break; // Found closing quote
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

int main() {
    FILE *file = fopen("parcels.csv", "r");
    if (!file) {
        printf("ERROR: Could not open parcels.csv\n");
        return 1;
    }

    CityNode *root = NULL;
    char line[2048];
    fgets(line, sizeof(line), file); // Skip header

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '\n' || line[0] == '\r') continue;
        
        char f[14][MAX_FIELD];
        int pos = 0;
        
        // Safely extract all columns while respecting quotes
        for (int i = 0; i < 14; i++) {
            pos = parse_field(line, pos, f[i], MAX_FIELD);
        }
        
        // Column 4 is Sender City, Column 8 is Receiver City
        if (strlen(f[4]) > 0) root = insert_city(root, f[4]);
        if (strlen(f[8]) > 0) root = insert_city(root, f[8]);
    }
    fclose(file);
    
    print_cities_inorder(root); 
    return 0;
}