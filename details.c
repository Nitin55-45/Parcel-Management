#define _CRT_SECURE_NO_WARNINGS
/* details.c — Collect and validate sender/receiver input */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "details.h"

void title_case(char *s) {
    int nw = 1;
    for (; *s; s++) {
        if (*s == ' ') nw = 1;
        else if (nw) { *s = toupper((unsigned char)*s); nw = 0; }
        else *s = tolower((unsigned char)*s);
    }
}
int is_non_empty(const char *s) { return s[0] != '\0'; }
int is_valid_phone(const char *s) {
    int len = 0;
    for (int i = 0; s[i]; i++) { if (!isdigit((unsigned char)s[i])) return 0; len++; }
    return len == 10;
}
void trim_newline(char *s) { size_t l = strlen(s); if (l > 0 && s[l-1] == '\n') s[l-1] = '\0'; }

void input_person(Person *p, const char *role) {
    printf("\n--- %s details ---\n", role);
    do { printf("Name : "); scanf(" %[^\n]", p->name);
         if (!is_non_empty(p->name)) printf(" -> Name cannot be empty.\n");
    } while (!is_non_empty(p->name));
    do { printf("Address : "); scanf(" %[^\n]", p->address);
         if (!is_non_empty(p->address)) printf(" -> Address cannot be empty.\n");
    } while (!is_non_empty(p->address));
    do { printf("City : "); scanf(" %[^\n]", p->city);
         if (!is_non_empty(p->city)) printf(" -> City cannot be empty.\n");
    } while (!is_non_empty(p->city));
    title_case(p->city);
    do { printf("Phone : "); scanf(" %[^\n]", p->phone);
         if (!is_valid_phone(p->phone)) printf(" -> Phone must be 10 digits.\n");
    } while (!is_valid_phone(p->phone));
}

void input_parcel_info(ParcelInfo *pi) {
    printf("\n--- Parcel details ---\n");
    do { printf("Weight (kg) : "); scanf("%f", &pi->weight);
         if (pi->weight <= 0) printf(" -> Weight must be positive.\n");
    } while (pi->weight <= 0);
    do { printf("Type (docs/box/etc.) : "); scanf(" %[^\n]", pi->type);
         if (!is_non_empty(pi->type)) printf(" -> Type cannot be empty.\n");
    } while (!is_non_empty(pi->type));
    printf("Special instructions (optional) : "); scanf(" %[^\n]", pi->specialInstructions);
    printf("Priority (0:Std, 1:Exp) : "); scanf("%d", &pi->priority);
}