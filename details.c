// details.c
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "details.h"

int is_non_empty(const char *s) {
    return s[0] != '\0';
}

int is_valid_phone(const char *s) {
    int len = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (!isdigit((unsigned char)s[i]))
            return 0;
        len++;
    }
    return (len ==10 );
}

void trim_newline(char *s) {
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n')
        s[len - 1] = '\0';
}

void input_person(Person *p, const char *role) {
    printf("\n--- %s details ---\n", role);

    do {
        printf("Name    : ");
        scanf(" %[^\n]", p->name);
        if (!is_non_empty(p->name))
            printf("  -> Name cannot be empty. Please re-enter.\n");
    } while (!is_non_empty(p->name));

    do {
        printf("Address : ");
        scanf(" %[^\n]", p->address);
        if (!is_non_empty(p->address))
            printf("  -> Address cannot be empty. Please re-enter.\n");
    } while (!is_non_empty(p->address));

    do {                                                          
        printf("City    : ");                                     
        scanf(" %[^\n]", p->city);                               
        if (!is_non_empty(p->city))                             
            printf("  -> City cannot be empty. Please re-enter.\n"); 
     } while (!is_non_empty(p->city));

    do {
        printf("Phone   : ");
        scanf(" %[^\n]", p->phone);
        if (!is_valid_phone(p->phone))
            printf("  -> Phone must be 10 digits only. Please re-enter.\n");
    } while (!is_valid_phone(p->phone));
}

void input_parcel_info(ParcelInfo *pi) {
    printf("\n--- Parcel details ---\n");

    do {
        printf("Weight (kg)          : ");
        scanf("%f", &pi->weight);
        if (pi->weight <= 0)
            printf("  -> Weight must be positive. Please re-enter.\n");
    } while (pi->weight <= 0);
     
    do {
        printf("Type (docs/box/etc.) : ");
        scanf(" %[^\n]", pi->type);
        if (!is_non_empty(pi->type))
            printf("  -> Type cannot be empty. Please re-enter.\n");
    } while (!is_non_empty(pi->type));

    printf("Special instructions (optional) : ");
    scanf(" %[^\n]", pi->specialInstructions);
}
