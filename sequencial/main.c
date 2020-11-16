#include <unistd.h>
#include <stdlib.h>

#define NUMBER_BUCKET 10

struct Bucket {
    int* array;
    size_t n_elem;
    size_t max_elem;
};

struct Bucket make(size_t max){
    struct Bucket block_arr =  {
        .array = malloc(sizeof(int) * max),
        .n_elem = 0,
        .max_elem = max,
    };

    return block_arr;
}

void insert(struct Bucket* arr, int elem){
    if(arr->n_elem >= arr->max_elem){
        arr->array = realloc(arr->array, sizeof(int) * arr->max_elem * 2);
        arr->max_elem *= 2;
     }
     
     arr->array[arr->n_elem] = elem;
     arr->n_elem++;
}

void bucket_sort(size_t size, int arr[size]){
    int max, min;
    
    struct Bucket buckets[NUMBER_BUCKET];
    for (size_t i = 0; i < NUMBER_BUCKET; i++){
        buckets[i] = make(size);
    }

    for (size_t i = 0; i < size; i++){
        NUMBER_BUCKET / (max - min) * arr[i]
    }
    

} 

int main(){

}
