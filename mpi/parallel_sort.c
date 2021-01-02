#include <mpi.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

#define N_BUCKETS 10

struct Bucket {
    int* array;
    size_t n_elem;
    size_t max_elem;
};

enum STATS {
    MIN = 0,
    MAX = 1,
    BUF = 2,
};

struct Bucket* make(size_t max){
    struct Bucket* block_arr = malloc(sizeof(struct Bucket));
    block_arr->array = malloc(sizeof(int) * max);
    block_arr->n_elem = 0;
    block_arr->max_elem = max;

    return block_arr;
}

void print_arr(int* arr, size_t size){
    for(size_t i = 0; i < size; i++) {
        arr[i] = arr[i];
        printf("%d\n", arr[i]);
    }
}

void insert(struct Bucket* arr, int elem){
    if(arr->n_elem >= arr->max_elem){
        arr->array = realloc(arr->array, sizeof(int) * arr->max_elem * 2);
        arr->max_elem *= 2;
    }
    arr->array[arr->n_elem] = elem;
    arr->n_elem++;
}

void build_buckets(int* arr, int size, struct Bucket* out_buckets[N_BUCKETS], int stats[3]) {
    for (size_t i = 0; i < N_BUCKETS; i++)
        out_buckets[i] = make(size);

    for (size_t i = 0; i < size; i++){
        size_t n_bucket = (arr[i] + abs(stats[MIN])) * N_BUCKETS / (abs(stats[MAX] + abs(stats[MIN])));
        n_bucket = n_bucket >= N_BUCKETS ? N_BUCKETS - 1 : n_bucket;

#ifdef NDEBUG
        printf("%zu<- %d\n", n_bucket, arr[i]);
#endif

        insert(out_buckets[n_bucket], arr[i]);
    }
}

int cmpfunc (const void * a, const void * b) {
    return ( *(int*)a - *(int*)b );
}

int main( int argc, char *argv[]) { 
    int nrank, rank, msg;
    //[Min, Max, Buffer_Size
    int stats[3];
    MPI_Status status; 

    MPI_Init(&argc, &argv); 
    MPI_Comm_rank( MPI_COMM_WORLD, &rank ); 
    MPI_Comm_size( MPI_COMM_WORLD, &nrank ); 
    struct Bucket* b = NULL;
    struct Bucket* buckets[N_BUCKETS];

    if (rank == 0) { 
        //Read data
        b = make(100);
        FILE* file = fopen (argv[1], "r");
        int v;
        while (!feof (file)){
            fscanf (file, "%d\n", &v);
            insert(b, v);
        }
        stats[BUF] = 1 + b->n_elem / (nrank - 1);
        stats[MAX] = stats[MIN] = b->array[0];

        //Calculate Max and min
        for(size_t i = 1; i < b->n_elem; i++) {
            if(b->array[i] > stats[MAX]) stats[MAX] = b->array[i];
            if(b->array[i] < stats[MIN]) stats[MIN] = b->array[i];
        }
    } 
    //Broadcast Min, max and max buffer size
    MPI_Bcast(&stats, 3, MPI_INT, 0, MPI_COMM_WORLD);
    if(!rank) {
        //Calculate how much elements to send to each process
        //paying attention to unbalanced buckets
        int base_send = b->n_elem / (nrank - 1);
        int reminder = b->n_elem % (nrank - 1);
        for(size_t i = 1, curr_pos = 0; i < nrank; i++) {
            int send_quantity = base_send + (reminder-- > 0 ? 1 : 0);
            int send_position = curr_pos;
            curr_pos += send_quantity;
            //Send work to each processes
            //This could be done with Scatter_v, but it doesnt give us the status from
            //the communication
            MPI_Send(b->array + send_position, send_quantity, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    }
    if (rank) { 
        //Recive elems
        int buffer[stats[BUF]];
        int real_size;
        MPI_Recv(buffer, stats[BUF], MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        //Calculate the real number of elems recived
        MPI_Get_count(&status, MPI_INT, &real_size);
        //Build buckets
        build_buckets(buffer, real_size, buckets, stats);
        //Send buckets to root
        for(size_t i = 0; i < N_BUCKETS; i++) {
            MPI_Send(buckets[i]->array, buckets[i]->n_elem, MPI_INT, 0, i, MPI_COMM_WORLD);
        }
    }
    //Join Buckets
    if(!rank) {
        for (size_t i = 0; i < N_BUCKETS; i++)
            buckets[i] = make(b->n_elem);
        int buffer[stats[BUF]];
        for(size_t i = 0; i < (nrank-1) * N_BUCKETS; i++) {
            MPI_Recv(buffer, stats[BUF], MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            int real_size;
            MPI_Get_count(&status, MPI_INT, &real_size);
            memcpy(buckets[status.MPI_TAG]->array + buckets[status.MPI_TAG]->n_elem, buffer, real_size * sizeof(int));
            buckets[status.MPI_TAG]->n_elem += real_size;
        }
        //Sort Buckets
        for (size_t i = 0; i < N_BUCKETS; i++)
        {
            if (buckets[i]->n_elem > 1)
                qsort(buckets[i]->array, buckets[i]->n_elem, sizeof(int), cmpfunc);
        }

        //Join Array
        size_t i = 0, j;
        for(j = 0; j < N_BUCKETS; j++) {
            memcpy(b->array + i, buckets[j]->array, buckets[j]->n_elem * sizeof(int));
            i += buckets[j]->n_elem;
        }

        print_arr(b->array, b->n_elem);
    }

    /*
    //Scatter doesn't give the mpi stat param, this meaning we cant know the size of the message
    MPI_Scatter(b->array, b->n_elem, MPI_INT, &buffer, stats[BUF], MPI_INT, 0, MPI_COMM_WORLD);
    //Build buckets
    MPI_Gather(buffer, stats[BUF], MPI_INT, &b->array, stats[BUF], MPI_INT, 0, MPI_COMM_WORLD);
    else if (rank == nrank - 1) { 
//Join buckets
MPI_Recv( &msg, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &status ); 
printf( "Received on %d: %d\n", rank, msg); 
} 
else { 
    //Build results
    MPI_Recv( &msg, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &status ); 
    printf( "Received on %d: %d\n", rank, msg); 
    MPI_Send( &msg, msg, MPI_INT, rank+1, 0, MPI_COMM_WORLD); 
    } 
    */

MPI_Finalize(); 

return 0; 
} 
