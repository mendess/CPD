#include "common/debug.h"
#include "mpi/util.h"

void swap(Matrix* const a, Matrix* const b) {
    Matrix tmp = *a;
    *a = *b;
    *b = tmp;
}

size_t
start_chunk(size_t const proc_id, int const nprocs, size_t const num_iters) {
    size_t const rem = (num_iters % nprocs);
    size_t const x = proc_id * (num_iters - rem) / nprocs;
    return x + (proc_id < rem ? proc_id : rem);
}

size_t
proc_from_chunk(size_t const k, int const nprocs, size_t const num_iters) {
    for (int i = 0; i < nprocs; ++i) {
        size_t try_k = start_chunk(i, nprocs, num_iters);
        size_t try_k_end = start_chunk(i + 1, nprocs, num_iters);
        if (try_k <= k && k < try_k_end) {
            return i;
        }
    }
    eprintf("k = %zu\n", k);
    debug_print_backtrace("k out of range");
}

Slice slice_rows(int const proc_id, int const nprocs, size_t const n_rows) {
    return (Slice){
        .start = start_chunk(proc_id, nprocs, n_rows),
        .end = start_chunk(proc_id + 1, nprocs, n_rows)};
}

size_t slice_len(int const proc_id, int const nprocs, size_t const n_rows) {
    return start_chunk(proc_id + 1, nprocs, n_rows) -
           start_chunk(proc_id, nprocs, n_rows);
}
