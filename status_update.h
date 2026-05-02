#ifndef STATUS_UPDATE_H
#define STATUS_UPDATE_H

#define STATUS_BOOKED            "Booked"
#define STATUS_INTRANSIT         "InTransit"
#define STATUS_OUT_FOR_DELIVERY  "Out for Delivery"
#define STATUS_DELIVERED         "Delivered"

int is_valid_status(const char *status);
int save_status_to_csv(const char *tracking, const char *status,
                       const char *staff_id, const char *staff_name,
                       const char *location);
#endif