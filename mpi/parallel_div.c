#include <mpi.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

#define N_BUCKETS 10

//TODO: This isnt very nice for when buck < procs
//TODO: Fix mem allocation
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

struct Bucket* read_from_file(char* fname) {
    //Read data
    struct Bucket* b = make(100);
    FILE* file = fopen (fname, "r");
    int v;
    while (!feof (file)){
        fscanf (file, "%d\n", &v);
        insert(b, v);
    }
    return b;
}

int main( int argc, char *argv[]) { 
    int nrank, rank, msg;
    //[Min, Max, Buffer_Size]
    int stats[3];
    MPI_Status status; 

    MPI_Init(&argc, &argv); 
    MPI_Comm_rank( MPI_COMM_WORLD, &rank ); 
    MPI_Comm_size( MPI_COMM_WORLD, &nrank ); 
    struct Bucket* b = NULL;
    struct Bucket* buckets[N_BUCKETS];

    if (!rank) { 
        b = read_from_file(argv[1]);
        //Calculate the max number of elements sent to each process
        stats[BUF] = 1 + b->n_elem / nrank;

        //Calculate Max and min
        stats[MAX] = stats[MIN] = b->array[0];
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
        int base_send = b->n_elem / nrank;
        int reminder = b->n_elem % nrank;
        for(size_t i = 0, curr_pos = 0; i < nrank; i++) {
            int send_quantity = base_send + (reminder-- > 0 ? 1 : 0);
            int send_position = curr_pos;
            curr_pos += send_quantity;
            //Send work to each processes
            //This could be done with Scatter_v, but it doesnt give us the status from
            //the communication
            MPI_Send(b->array + send_position, send_quantity, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    }
    //Recive elems
    int buffer[stats[BUF]];
    int real_size;
    fprintf(stderr, "Recive buckets in %d\n", rank);
    MPI_Recv(buffer, stats[BUF], MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    //Calculate the real number of elems recived
    MPI_Get_count(&status, MPI_INT, &real_size);
    //Build buckets
    build_buckets(buffer, real_size, buckets, stats);
    //Send buckets to correct process
    int curr_buck = 0;
    for (size_t p = 0; p < nrank; p++) {
        int n_buckets = nrank >= N_BUCKETS ? 1 : (N_BUCKETS / nrank);
        int mod = N_BUCKETS % nrank;
        n_buckets += mod && p < mod ? 1 : 0;
        for(size_t i = 0; i < n_buckets; i++) {
            fprintf(stderr, "Send bucket %d from %d to %zu\n", curr_buck, rank, p);
            MPI_Send(buckets[curr_buck]->array, buckets[curr_buck]->n_elem, MPI_INT, p, curr_buck, MPI_COMM_WORLD);
            curr_buck++;
        }
    }
    //Process a bucket if applicable
    if(rank <= N_BUCKETS) {
        //Determine buckets that it will recive
        int mod = N_BUCKETS % nrank;
        int n_buckets = nrank >= N_BUCKETS ? 1 : (N_BUCKETS / nrank);
        n_buckets += rank + 1 <= mod ? 1 : 0;
        //Offset to properly convert min_bucket to max_bucket into 0 to n_bucket
        int offset = rank <= mod ? rank : mod;
        fprintf(stderr, "%d %d will recive %d buckets\n", offset, rank, n_buckets);
        //Make them
        for (size_t i = 0; i < n_buckets; i++)
            buckets[i] = make(stats[BUF]);
        int total_recived = 0;
        for(size_t i = 0; i < n_buckets * nrank; i++) {
            //Recive them
            MPI_Recv(buffer, stats[BUF], MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            fprintf(stderr, "Recive %d from %d in %d\n", status.MPI_TAG, status.MPI_SOURCE, rank);
            int real_size;
            MPI_Get_count(&status, MPI_INT, &real_size);
            int buck = (status.MPI_TAG + offset) % n_buckets;
            //Save the recived bucket
            memcpy(buckets[buck]->array + buckets[buck]->n_elem, buffer, real_size * sizeof(int));
            buckets[buck]->n_elem += real_size;
            total_recived += real_size;
        }
        //Sort Buckets
        for (size_t i = 0; i < n_buckets; i++)
        {
            printf("-----Bucket %zu in %d rank-----\n", i, rank);
            print_arr(buckets[i]->array, buckets[i]->n_elem);
            printf("----------\n");
            fprintf(stderr, "Sort %zu in %d\n", i, rank);
            if (buckets[i]->n_elem > 1)
                qsort(buckets[i]->array, buckets[i]->n_elem, sizeof(int), cmpfunc);
        }
        //Join Array
        struct Bucket* c = make(total_recived);
        for(size_t j = 0; j < n_buckets; j++) {
            fprintf(stderr, "Join %zu in %d\n", j, rank);
            memcpy(c->array + c->n_elem, buckets[j]->array, buckets[j]->n_elem * sizeof(int));
            c->n_elem += buckets[j]->n_elem;
        }
        fprintf(stderr, "Send final array%d\n", rank);
        //Send the sorted and joined array to master
        MPI_Send(c->array, c->n_elem, MPI_INT, 0, 0, MPI_COMM_WORLD);
        fprintf(stderr, "Sent final array%d\n", rank);
    }
    if(!rank) {
        int max_size = b->n_elem;
        b->n_elem = 0;
        //Gather sorted results
        for(int i = 0; i < nrank; i++) {
            int buffer[max_size], real_size;
            MPI_Recv(buffer, max_size, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            MPI_Get_count(&status, MPI_INT, &real_size);
            for(int z = 0; z < real_size; printf("%d ", buffer[z++]));
            printf("\n");
            fprintf(stderr, "--Recive %d from %d\n", real_size, status.MPI_SOURCE);
            memcpy(b->array + b->n_elem, buffer, real_size * sizeof(int));
            b->n_elem += real_size;
        }
        //Print them
        printf("------------\n");
        printf("------------\n");
        print_arr(b->array, b->n_elem);
        printf("------------\n");
        printf("------------\n");
    }

    MPI_Finalize(); 

    return 0; 
} 
