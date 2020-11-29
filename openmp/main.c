#include <unistd.h>
#include <omp.h>
#include <stdlib.h>
#include <stdio.h>

#define N_BUCKETS 10

struct Bucket {
    int* array;
    size_t n_elem;
    size_t max_elem;
};


struct Bucket* make(size_t max){
    struct Bucket* block_arr = malloc(sizeof(struct Bucket));
    block_arr->array = malloc(sizeof(int) * max);
    block_arr->n_elem = 0;
    block_arr->max_elem = max;

    return block_arr;
}

void free_bucket(struct Bucket* b){
    free(b->array);
    free(b);
}

void buckets_to_arr(size_t size, struct Bucket* arr[size], int* res) {
    size_t w = 0;
    for(size_t bucket = 0; bucket < size; bucket++) {
#ifdef NDEBUG
        fprintf(stderr, "%zu: ", arr[bucket]->n_elem);
#endif
        for(size_t elem = 0; elem < arr[bucket]->n_elem; elem++) { //vectorized
            res[w++] = arr[bucket]->array[elem];
#ifdef NDEBUG
            fprintf(stderr, "%d ", arr[bucket]->array[elem]);
#endif
        }
#ifdef NDEBUG
        fprintf(stderr, "\n");
#endif
    }
}

void insert(struct Bucket* arr, int elem){
    if(arr->n_elem >= arr->max_elem){ //SIMD doesnt like this if
        arr->array = realloc(arr->array, sizeof(int) * arr->max_elem * 2);
        arr->max_elem *= 2;
    }
    arr->array[arr->n_elem] = elem;
    arr->n_elem++;
}


int cmpfunc (const void * a, const void * b) {
    return ( *(int*)a - *(int*)b );
}

void bucket_sort(size_t size, int* arr){
    if (!size){
        fprintf(stderr, "Can't calculate max of empty array");
        _exit(1);
    }

    int max = arr[0];
    int min = arr[0];
    for(size_t i = 1; i < size; i++) { //vectorized
        if(arr[i] > max) max = arr[i];
        if(arr[i] < min) min = arr[i];
    }

#ifdef NDEBUG
    fprintf(stderr, "max: %d, min: %d\n", max, min);
#endif

    struct Bucket* buckets[N_BUCKETS];
    for (size_t i = 0; i < N_BUCKETS; i++)
        buckets[i] = make(size);
#pragma omp parallel
    {
#pragma omp for
    for (size_t i = 0; i < N_BUCKETS; i++) {
        for (size_t j = 0; j < size; j++){
            size_t n_bucket = (arr[j] + abs(min)) * N_BUCKETS / (abs(max + abs(min)));
            n_bucket = n_bucket >= N_BUCKETS ? N_BUCKETS - 1 : n_bucket;
            if(n_bucket == i) {
#ifdef NDEBUG
                fprintf(stderr, "%zu<- %d\n", i, arr[j]);
#endif
                buckets[i]->array[buckets[i]->n_elem] = arr[j];
                buckets[i]->n_elem++;
            }
        }
    }

#pragma omp barrier
#pragma omp for
    for (size_t i = 0; i < N_BUCKETS; i++)
        qsort(buckets[i]->array, buckets[i]->n_elem, sizeof(int), cmpfunc);
    }
    buckets_to_arr(N_BUCKETS, buckets, arr);

    for (size_t i = 0; i < N_BUCKETS; i++)
        free_bucket(buckets[i]);

}

void print_arr(int* arr, size_t size){
    for(size_t i = 0; i < size; i++) {
        arr[i] = arr[i];
        printf("%d\n", arr[i]);
    }
}


int main(int argc, char** argv){
    struct Bucket* b = make(100);

    FILE* file = fopen (argv[1], "r");
    int v;
    while (!feof (file)){
        fscanf (file, "%d\n", &v);
        insert(b, v);
    }

    size_t size = b->n_elem;

#ifdef NDEBUG
    print_arr(b->array, size);
#endif

    double time = omp_get_wtime(); 
    bucket_sort(size, b->array);
    fprintf(stderr, "Time=%f\n", omp_get_wtime()-time); 

    print_arr(b->array, size);

    free_bucket(b);

}
