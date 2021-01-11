#include "stats.h"
#include <stdio.h>
#include<stdlib.h>
#include<unistd.h>

stats stats_new(const int init_min, const int init_max, const int init_buf_size, const int total_read) {
    stats r;
    r.max = init_max;
    r.min = init_min;
    r.buf_size = init_buf_size;
    r.total_read = total_read;
    return r;
}

void stats_update_max(stats* stats, const int new_max) {
    stats->max = new_max > stats->max ? new_max : stats->max;
}
    
void stats_update_min(stats* stats, const int new_min) {
    stats->min = new_min < stats->min ? new_min : stats->min;
}

void stats_update_min_max(stats* stats, const int new_val) {
    stats->min = new_val < stats->min ? new_val : stats->min;
    stats->max = new_val > stats->max ? new_val : stats->max;
}

void stats_print(const stats* stats, FILE* f) {
    fprintf(f, "%d: Min = %d, Max = %d, Buf_Size = %d, Total_Read = %d\n", 
            getpid(), stats->min, stats->max, stats->buf_size, stats->total_read);
}

int* stats_as_arr(const stats* stats) {
    int* r = malloc(sizeof(int) * 4);
    r[0] = stats->max;
    r[1] = stats->min;
    r[2] = stats->buf_size;
    r[3] = stats->total_read;
    return r;
}
stats stats_from_arr(const int* r) {
    return stats_new(r[1], r[0], r[2], r[3]);
}
