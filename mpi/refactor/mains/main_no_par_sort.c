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

buckets bucket_recv(const size_t n_buckets, const size_t size, const size_t proc_units, const size_t proc_unit, const size_t rank_size, const size_t root) {
    MPI_Status status;
    buckets r = buckets_new(n_buckets, size);
    int* buffer = malloc(size * sizeof(int));
    for(size_t i = 0; i < n_buckets * rank_size; i++) {
        int s;
        MPI_Recv(buffer, size, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_INT, &s);
        if(s) {
#ifdef NDEBUG
            fprintf(stderr, "%d: Recv Buck %d from %d with %d elems saved in buck %d\n", getpid(), status.MPI_TAG, status.MPI_SOURCE, s, buck);
#endif
            bucket_append(&r, status.MPI_TAG, buffer, s);
        }
    }
    return r;
}

void bucket_send(const buckets* b, const size_t proc_units, const int rank) {
    MPI_Request r;
    for(size_t i = 0; i < b->size; i++) {
        bucket* tmp = buckets_get(b, i);
#ifdef NDEBUG
        fprintf(stderr, "%d: Send Buck %d to %zu with %d elems\n", getpid(), last_bucket, p, tmp->len);
#endif
        MPI_Isend(tmp->array, tmp->len, MPI_INT, 0, i, MPI_COMM_WORLD, &r);
#ifdef NDEBUG
        fprintf(stderr, "%d: --Send Buck %d to %zu with %d elems\n", getpid(), last_bucket, p, tmp->len);
#endif
    }
}

int main(int argc, char** argv) {
    int rank, size_rank;
    int n_buckets = 10;
    stats stats;
    dyn_arr* chunks;
    dyn_arr buf;
    MPI_Status status;
    double time;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size_rank);
#ifdef NDEBUG
    fprintf(stderr, "rank %d -> pid %d\n", rank, getpid());
#endif
    if(!rank) {
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
    bucket_send(&b, proc_units, 0);
#ifdef NDEBUG
    fprintf(stderr, "%d: --Send buckets\n", getpid());
#endif
    MPI_Barrier(MPI_COMM_WORLD);
    if(!rank) {
        //Recv buckets
        buckets r = bucket_recv(n_buckets, stats.buf_size, proc_units, rank, size_rank, 0);
        //Sort Buckets
        buckets_sort(&r);
        //Join to arr
        dyn_arr res = buckets_to_dyn_arr(&r);
        //Print Results
        fprintf(stderr, "Time = %f\n", MPI_Wtime() - time);
        dyn_arr_print(res, stdout, "\n");
    }
    MPI_Finalize();
}
