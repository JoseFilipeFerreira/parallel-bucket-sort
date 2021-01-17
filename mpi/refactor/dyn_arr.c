#include "dyn_arr.h"
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>

int cmpfunc (const void * a, const void * b) {
    return ( *(int*)a - *(int*)b );
}

dyn_arr dyn_arr_new(const int size) {
    dyn_arr r;
    r.array = malloc(sizeof(int) * size);
    r.len = 0;
    r.capacity = size;
    return r;
}

void dyn_arr_init(dyn_arr* r, const int size) {
    r->array = malloc(sizeof(int) * size);
    r->len = 0;
    r->capacity = size;
}

dyn_arr dyn_arr_new_with_arr(const int* arr, const int size, const int capacity) {
    dyn_arr r;
    r.array = malloc(sizeof(int) * capacity + size);
    memcpy(r.array, arr, sizeof(int) * size);
    r.len = size;
    r.capacity = capacity;
    return r;
}

void dyn_arr_push(dyn_arr* arr, const int elem) {
    if(arr->capacity == arr->len) {
        arr->array = realloc(arr->array, 2*arr->capacity * sizeof(int));
        arr->capacity *= 2;
    }
    arr->array[arr->len++] = elem;
}

int dyn_arr_pop(dyn_arr* arr) {
    if(!arr->capacity) {
        fprintf(stderr, "%d: Pop on empty array\n", getpid());
        exit(EXIT_FAILURE);
    }
    return arr->array[arr->len--];
}

void dyn_arr_print(const dyn_arr arr, FILE* buff, const char* separator) {
    for(int i = 0; i < arr.len; fprintf(buff, "%d: %d%s", getpid(), arr.array[i++], separator));
}

dyn_arr dyn_arr_from_file(const char* fname) {
    dyn_arr r = dyn_arr_new(100);
    FILE* file = fopen(fname, "r");
    int tmp;
    if(!file) {
        perror("Couldnt load file");
        exit(EXIT_FAILURE);
    }
    while(!feof(file)) {
        fscanf (file, "%d\n", &tmp);
        dyn_arr_push(&r, tmp);
    }
    return r;
}

int* dyn_arr_get(const dyn_arr* arr, const int index) {
    if(index >= arr->capacity) {
        fprintf(stderr, "%d: Dyn_Arr: Index out of bounds %d of %d\n", getpid(), index, arr->len);
        exit(EXIT_FAILURE);
    }
    return &arr->array[index];
}

dyn_arr* dyn_arr_chunks(const dyn_arr* arr, int n_chunks, int* max_chunk_size) {
        int base_chunk = arr->len / n_chunks;
        int reminder = arr->len % n_chunks;
        *max_chunk_size = base_chunk;
        *max_chunk_size += reminder ? 1 : 0;
        dyn_arr* r = malloc(sizeof(dyn_arr) * n_chunks);
        for(size_t i = 0, curr_pos = 0; i < n_chunks; i++) {
            int chunk_size = base_chunk + (reminder-- > 0 ? 1 : 0);
            int read_position = curr_pos;
            curr_pos += chunk_size;
            r[i] = dyn_arr_new_with_arr(arr->array + read_position, chunk_size, chunk_size);
        }
        return r;
}

void dyn_arr_append(dyn_arr* old, const dyn_arr new) {
    old->array = realloc(old->array, (old->capacity + new.len) * sizeof(int));
    memcpy(old->array + old->len, new.array, new.len * sizeof(int));
    old->len += new.len;
    old->capacity += new.len;
}

void dyn_arr_append_arr(dyn_arr* old, const int* new, const int size) {
    old->array = realloc(old->array, (old->capacity + size) * sizeof(int));
    memcpy(old->array + old->len, new, size * sizeof(int));
    old->len += size;
    old->capacity += size;
}

void dyn_arr_sort(dyn_arr* arr) {
    qsort(arr->array, arr->len, sizeof(int), cmpfunc);
}

void dyn_arr_destroy(dyn_arr* arr) {
    free(arr->array);
}
