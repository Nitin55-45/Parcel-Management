// details.h
typedef struct {
    char name[50];
    char address[100];
    char city[50];
    char phone[15];
} Person;

typedef struct {
    float weight;
    char type[30];
    char specialInstructions[100];
} ParcelInfo;

// only these 2 are exposed to other modules
void input_person(Person *p, const char *role);
void input_parcel_info(ParcelInfo *pi);