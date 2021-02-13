#include<stdlib.h>
#include<stdio.h>
#include<string.h>

typedef struct _dyn_arr {
    int* array;
    int len; 
    int capacity;
} dyn_arr;

dyn_arr dyn_arr_new(const int size);
dyn_arr dyn_arr_new_with_arr(const int* arr, const int size, const int capacity);
void dyn_arr_init(dyn_arr* r, const int size);
void dyn_arr_push(dyn_arr* arr, const int elem);
int dyn_arr_pop(dyn_arr* arr);
void dyn_arr_print(const dyn_arr, FILE*, const char*);
dyn_arr dyn_arr_from_file(const char* fname);
int* dyn_arr_get(const dyn_arr* arr, const int index);
dyn_arr* dyn_arr_chunks(const dyn_arr* arr, const int n_chunks, int* max_chunk_size);
void dyn_arr_append(dyn_arr*, const dyn_arr);
void dyn_arr_append_arr(dyn_arr*, const int*, const int);
void dyn_arr_destroy(dyn_arr* arr);
void dyn_arr_sort(dyn_arr* arr);
