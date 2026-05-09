#ifndef DETAILS_H
#define DETAILS_H

typedef struct {
    char name[100];
    char address[200];
    char city[50];
    char phone[15];
} Person;

typedef struct {
    float weight;
    char type[50];
    char specialInstructions[200];
    int priority;
} ParcelInfo;

/* ── Function declarations ── */
void title_case(char *s);          /* ← add this */
int  is_non_empty(const char *s);
int  is_valid_phone(const char *s);
void input_person(Person *p, const char *role);
void input_parcel_info(ParcelInfo *pi);

#endif