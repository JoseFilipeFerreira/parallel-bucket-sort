#include<stdlib.h>
#include <stdio.h>

typedef struct _stats { 
    int max, min, buf_size, total_read; 
} stats;

stats stats_new(const int init_min, const int init_max, const int init_buf_size, const int total_read);
void stats_update_min_max(stats* stats, const int new_val);
void stats_print(const stats* stats, FILE* f);
int* stats_as_arr(const stats*);
stats stats_from_arr(const int*);
