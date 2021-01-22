#include<mpi.h>
#include <stdio.h>
#include<unistd.h>
#include "../buckets.h"

void send_dyn_arr(const dyn_arr* arr, const int dest, const int tag) {
    MPI_Request r;
#ifdef NDEBUG
    fprintf(stderr, "%d: Send Chunk to %d with %d elems\n", getpid(), dest, arr->len);
#endif
    MPI_Isend(arr->array, arr->len, MPI_INT, dest, tag, MPI_COMM_WORLD, &r);
#ifdef NDEBUG
    fprintf(stderr, "%d: Sent\n", getpid());
#endif
}

dyn_arr recive_dyn_arr(const int source, const int tag, const size_t size, MPI_Status* status) {
    dyn_arr r = dyn_arr_new(size);
#ifdef NDEBUG
    fprintf(stderr, "%d: Start Recv Chunk with %zu elems\n", getpid(), size);
#endif
    MPI_Recv(r.array, size, MPI_INT, source, tag, MPI_COMM_WORLD, status);
    MPI_Get_count(status, MPI_INT, &r.len);
#ifdef NDEBUG
    fprintf(stderr, "%d: Recv chunk %d\n", getpid(), r.len);
#endif
    return r;
}

buckets balanced_bucket_recv(const size_t n_buckets, const size_t size, const size_t proc_units, const size_t proc_unit, const size_t rank_size, const size_t root) {
    MPI_Status status;
    int recv_buckets = n_buckets / proc_units;
    int mod = n_buckets % proc_units;
    int offset = proc_unit < mod ? 0 : mod;
    recv_buckets += mod && proc_unit < mod ? 1 : 0;
    buckets r = buckets_new(recv_buckets, size);
    int* buffer = malloc(size * sizeof(int));
    for(size_t i = 0; i < recv_buckets * rank_size; i++) {
        int s;
        MPI_Recv(buffer, size, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_INT, &s);
        if(s) {
            int buck = (status.MPI_TAG - offset) % recv_buckets;
#ifdef NDEBUG
            fprintf(stderr, "%d: Recv Buck %d from %d with %d elems saved in buck %d\n", getpid(), status.MPI_TAG, status.MPI_SOURCE, s, buck);
#endif
            bucket_append(&r, buck, buffer, s);
        }
    }
    return r;
}

void balanced_bucket_send(const buckets* b, const size_t proc_units, const int rank) {
    MPI_Request r;
    int last_bucket = 0;
    for(size_t p = 0; p < proc_units; p++) {
        int send_buckets = b->size / proc_units;
        int mod = b->size % proc_units;
        send_buckets += mod && p < mod ? 1 : 0;
        for(size_t i = 0; i < send_buckets; i++) {
            bucket* tmp = buckets_get(b, last_bucket);
#ifdef NDEBUG
            fprintf(stderr, "%d: Send Buck %d to %zu with %d elems\n", getpid(), last_bucket, p, tmp->len);
#endif
            MPI_Isend(tmp->array, tmp->len, MPI_INT, p, last_bucket, MPI_COMM_WORLD, &r);
            last_bucket++;
#ifdef NDEBUG
            fprintf(stderr, "%d: --Send Buck %d to %zu with %d elems\n", getpid(), last_bucket, p, tmp->len);
#endif
        }
    }
}

int main(int argc, char** argv) {
    int rank, size_rank;
    stats stats;
    dyn_arr* chunks;
    dyn_arr buf;
    MPI_Status status;
    double time;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size_rank);
    int n_buckets = size_rank;
#ifdef NDEBUG
    fprintf(stderr, "rank %d -> pid %d\n", rank, getpid());
#endif

    if(rank == 0) {

        //Load elems to array
        buf = dyn_arr_from_file(argv[1]);
        time = MPI_Wtime();
        int max_chunk_size;
        //Build chunks to send to each process
        chunks = dyn_arr_chunks(&buf, size_rank, &max_chunk_size);
        //Calc min and max of the array
        stats = stats_new(buf.array[0], buf.array[0], max_chunk_size, buf.len);
        for(size_t i = 0; i < buf.len; i++)
            stats_update_min_max(&stats, *dyn_arr_get(&buf, i));
    }
#ifdef NDEBUG
    fprintf(stderr, "%d: Stats start\n", getpid());
#endif
    int* tmp_stats = !rank ? stats_as_arr(&stats) : malloc(sizeof(int) * 4);
    MPI_Bcast(tmp_stats, 4, MPI_INT, 0, MPI_COMM_WORLD);
    stats = stats_from_arr(tmp_stats);
    stats_print(&stats, stderr);
#ifdef NDEBUG
    fprintf(stderr, "%d: Stats end\n", getpid());
#endif
    if(!rank) {
        //Send chunks to each process
        for(size_t i = 0; i < size_rank; i++) {
            int z = (i + 1) % size_rank;
            send_dyn_arr(&chunks[z], z, 0);
        }
    }
    //Recive chunk to process
    dyn_arr to_process = recive_dyn_arr(0, MPI_ANY_TAG, stats.buf_size, &status);
    //Build buckets
#ifdef NDEBUG
    fprintf(stderr, "%d: Build buckets\n", getpid());
#endif
    buckets b = buckets_from_dyn_arr(&to_process, n_buckets, &stats);
#ifdef NDEBUG
    fprintf(stderr, "%d: --Build buckets\n", getpid());
#endif
    //Send Buckets to proper place
#ifdef NDEBUG
    fprintf(stderr, "%d: Send buckets\n", getpid());
#endif
    int proc_units = size_rank > n_buckets ? n_buckets : size_rank;
    balanced_bucket_send(&b, proc_units, 0);
#ifdef NDEBUG
    fprintf(stderr, "%d: --Send buckets\n", getpid());
#endif
    if(rank < proc_units) {
        //Recv buckets
        buckets r = balanced_bucket_recv(n_buckets, stats.buf_size, proc_units, rank, size_rank, 0);
        //Sort Buckets
        buckets_sort(&r);
        //Join to arr
        dyn_arr res = buckets_to_dyn_arr(&r);
        send_dyn_arr(&res, 0, 0);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if(!rank) {
        //Recive all buckets
        int recive_procs = size_rank > n_buckets ? n_buckets : size_rank;
        dyn_arr buff = dyn_arr_new(stats.total_read);
        for(size_t i = 0; i < recive_procs; i++) {
            const dyn_arr r = recive_dyn_arr(i, MPI_ANY_TAG, stats.total_read, &status); 
            dyn_arr_append(&buff, r);
        }
        //Print Results
        fprintf(stderr, "Time = %f\n", MPI_Wtime() - time);
        dyn_arr_print(buff, stdout, "\n");
    }
    MPI_Finalize();
}
