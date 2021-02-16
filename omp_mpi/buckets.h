#include "dyn_arr.h"
#include "stats.h"
#include <stdlib.h>

typedef struct _buckets {
    dyn_arr* buckets;
    int size;
} buckets;

typedef dyn_arr bucket;

buckets buckets_new(const int n_buckets, const int size);
bucket* buckets_get(const buckets* bucks, const int index); 
buckets buckets_from_dyn_arr(const dyn_arr* arr, const int n_buckets, const stats* stats);
dyn_arr buckets_to_dyn_arr(const buckets* b);
void buckets_sort(buckets* b);
void buckets_destroy(buckets);

void bucket_append(const buckets* bucks, const int index, int* buffer, int size);
