#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define NUMBER_BUCKET 10

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

//This could write in res with no limits
void buckets_to_arr(size_t size, struct Bucket* arr[size], int* res) {
    size_t w = 0;
    for(size_t bucket = 0; bucket < size; bucket++) {
        printf("%zu ", arr[bucket]->n_elem);
        for(size_t elem = 0; elem < arr[bucket]->n_elem; elem++) {
            res[w++] = arr[bucket]->array[elem];
            printf("%d ", arr[bucket]->array[elem]);
        }
        printf("\n");
    }
}

int array_max(int* arr, size_t size) {
    if(size > 0) {
        int curr_max = arr[0];
        for(size_t i = 1; i < size; i++)
            if(arr[i] > curr_max) curr_max = arr[i];
        return curr_max;
    }
    fprintf(stderr, "Can't calculate max of empty array");
    _exit(1);
}

void insert(struct Bucket* arr, int elem){
    if(arr->n_elem >= arr->max_elem){
        arr->array = realloc(arr->array, sizeof(int) * arr->max_elem * 2);
        arr->max_elem *= 2;
    }

    arr->array[arr->n_elem] = elem;
    arr->n_elem++;
}

void bucket_sort(size_t size, int* arr){
    int max = array_max(arr, size);

    struct Bucket* buckets[NUMBER_BUCKET];
    for (size_t i = 0; i < NUMBER_BUCKET; i++){
        buckets[i] = make(size);
    }

    for (size_t i = 0; i < size; i++){
        int n_bucket = max == 0 ? 0 : NUMBER_BUCKET * arr[i] / max;
        n_bucket = n_bucket == NUMBER_BUCKET ? n_bucket - 1 : n_bucket;
        insert(buckets[n_bucket], arr[i]);
    }

    for (size_t i = 0; i < NUMBER_BUCKET; i++){
        if(buckets[i]->n_elem > 1)
            bucket_sort(buckets[i]->n_elem, buckets[i]->array);
    }

    buckets_to_arr(NUMBER_BUCKET, buckets, arr);
} 

int main(){
    int* arr = malloc(sizeof(int) * 20);
    int arri[20] = {1, 10, 2, 35, 12, 4, 89, 131, 30, 0, 20, 11, 3, 36, 13, 5, 90, 14, 44, 55};

    for(size_t i = 0; i < 20; i++) {
        arr[i] = arri[i];
        printf("%d ", arr[i]);
    }
    printf("\n");
    bucket_sort(20, arr);
    for(size_t i = 0; i < 20; i++) 
        printf("%d ", arr[i]);
    printf("\n");

}
