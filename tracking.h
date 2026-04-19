#ifndef TRACKING_H
#define TRACKING_H

#define COUNTER_FILE "counter.txt"

int read_counter(char *last_date);
void write_counter(const char *date, int seq);
void make_suffix(char *suffix, int seed_extra);
void generate_tracking_number(char *out);

#endif
