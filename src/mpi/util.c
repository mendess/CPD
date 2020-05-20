#include "mpi/util.h"

#include "common/debug.h"

#include <stdbool.h>
#include <stdio.h>

unsigned NPROCS;
unsigned ME;
unsigned CHECKER_BOARD_SIDE;

void swap(Matrix* const a, Matrix* const b) {
    Matrix tmp = *a;
    *a = *b;
    *b = tmp;
}

void vswap(VMatrix* const a, VMatrix* const b) {
    VMatrix tmp = *a;
    *a = *b;
    *b = tmp;
}

bool should_work_alone(size_t const rows, size_t const columns) {
    return NPROCS == 1 || CHECKER_BOARD_SIDE > rows ||
           CHECKER_BOARD_SIDE > columns;
}

// [x,x,x,x,x,x,x,x,x,x] num_iters = 10
// proc = 0, nprocs = 3 => 0
// proc = 1, nprocs = 3 => 3
// proc = 2, nprocs = 3 => 6
// proc = 3, nprocs = 3 => 9
static inline size_t start_chunk(
    unsigned const proc, unsigned const nprocs, size_t const num_elems) {
    size_t const rem = num_elems % nprocs;
    size_t const x = proc * (num_elems - rem) / nprocs;
    return x + (proc < rem ? proc : rem);
}

static unsigned proc_from_chunk(
    size_t const index, unsigned const nprocs, size_t const num_elems) {
    // FIXME: Find a O(1) way to do this.
    for (unsigned i = 0; i < nprocs; ++i) {
        Slice try_bound = slice_rows(i, nprocs, num_elems);
        if (try_bound.start <= index && index < try_bound.end) {
            return i;
        }
    }
    eprintln("index = %zu", index);
    debug_print_backtrace("k out of range");
}

Slice slice_rows(
    unsigned const proc_id, unsigned const nprocs, size_t const n_rows) {
    return (Slice){
        .start = start_chunk(proc_id, nprocs, n_rows),
        .end = start_chunk(proc_id + 1, nprocs, n_rows)};
}

size_t
slice_len(unsigned const proc_id, unsigned const nprocs, size_t const n_rows) {
    return start_chunk(proc_id + 1, nprocs, n_rows) -
           start_chunk(proc_id, nprocs, n_rows);
}

ABounds a_bounds(unsigned const zone, size_t const rows, size_t const columns) {
    unsigned x = zone / CHECKER_BOARD_SIDE;
    unsigned y = zone % CHECKER_BOARD_SIDE;
    Slice i_bounds = slice_rows(x, CHECKER_BOARD_SIDE, rows);
    Slice j_bounds = slice_rows(y, CHECKER_BOARD_SIDE, columns);
    return (ABounds){.i = i_bounds, .j = j_bounds};
}

unsigned proc_from_row_column(
    size_t const row,
    size_t const column,
    size_t const rows,
    size_t const columns) {

    unsigned x = proc_from_chunk(row, CHECKER_BOARD_SIDE, rows);
    unsigned y = proc_from_chunk(column, CHECKER_BOARD_SIDE, columns);
    return x * CHECKER_BOARD_SIDE + y;
}
