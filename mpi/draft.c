#include <mpi.h> 
#include <stdio.h> 
#include <stdlib.h>

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

int main( int argc, char *argv[]) { 
    int nrank, rank, msg;
    //[Min, Max, Buffer_Size
    int stats[3] = {0};
    MPI_Status status; 

    MPI_Init(&argc, &argv); 
    MPI_Comm_rank( MPI_COMM_WORLD, &rank ); 
    MPI_Comm_size( MPI_COMM_WORLD, &nrank ); 
    struct Bucket* b = NULL;

    if (rank == 0) { 
        //Read data
        b = make(100);
        FILE* file = fopen (argv[1], "r");
        int v;
        while (!feof (file)){
            fscanf (file, "%d\n", &v);
            insert(b, v);
        }
        //Calculate how much elements to send to each process
        //paying attention to unbalanced buckets
        stats[BUF] = 1 + b->n_elem / (nrank - 1);
        int base_send = b->n_elem / (nrank - 1);
        int reminder = b->n_elem % (nrank - 1);
        int send_quantity[nrank];
        int send_position[nrank];
        send_position[0] = send_quantity[0] = 0;
        for(size_t i = 1, curr_pos = 0; i < nrank; i++) {
            send_quantity[i] = base_send + (reminder-- ? 1 : 0);
            send_position[i] = curr_pos;
            curr_pos += send_quantity[i];
        }
        
        //Calculate Max and min
        for(size_t i = 1; i < b->n_elem; i++) {
            if(b->array[i] > stats[MAX]) stats[MAX] = b->array[i];
            if(b->array[i] < stats[MIN]) stats[MIN] = b->array[i];
        }
    } 
    //Broadcast Min, max and max buffer size
    MPI_Bcast(&stats, 3, MPI_INT, 0, MPI_COMM_WORLD);

    //Split work between processes
    int buffer[stats[BUF]];
    //Scatter doesn't give the mpi stat param, this meaning we cant know the size of the message
    MPI_Scatter(b->array, b->n_elem, MPI_INT, &buffer, stats[BUF], MPI_INT, 0, MPI_COMM_WORLD);
    //Build buckets
    MPI_Gather(buffer, stats[BUF], MPI_INT, &b->array, stats[BUF], MPI_INT, 0, MPI_COMM_WORLD);
    /*
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
