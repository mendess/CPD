#include "mpi/util.h"

#include "common/debug.h"

#include <stdio.h>

int NPROCS;
int ME;
int CHECKER_BOARD_SIDE;

void swap(Matrix* const a, Matrix* const b) {
    Matrix tmp = *a;
    *a = *b;
    *b = tmp;
}

// [x,x,x,x,x,x,x,x,x,x] num_iters = 10
// proc = 0, nprocs = 3 => 0
// proc = 1, nprocs = 3 => 3
// proc = 2, nprocs = 3 => 6
// proc = 3, nprocs = 3 => 9
size_t start_chunk(int const proc, int const nprocs, size_t const num_elems) {
    size_t const rem = (num_elems % nprocs);
    size_t const x = proc * (num_elems - rem) / nprocs;
    return x + ((unsigned) proc < rem ? (unsigned) proc : rem);
}

int proc_from_chunk(
    size_t const index, int const nprocs, size_t const num_elems) {
    for (int i = 0; i < nprocs; ++i) {
        Slice try_bound = slice_rows(i, nprocs, num_elems);
        if (try_bound.start <= index && index < try_bound.end) {
            return i;
        }
    }
    eprintf("index = %zu\n", index);
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
