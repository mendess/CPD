#ifndef MPI_UTIL_H
#define MPI_UTIL_H
#include "common/matrix.h"

#include <stddef.h>

typedef struct {
    size_t start;
    size_t end;
} Slice;

void swap(Matrix* const a, Matrix* const b);
size_t
start_chunk(size_t const proc_id, int const nprocs, size_t const num_iters);

size_t
proc_from_chunk(size_t const k, int const nprocs, size_t const num_iters);
Slice slice_rows(int const proc_id, int const nprocs, size_t const n_rows);
size_t slice_len(int const proc_id, int const nprocs, size_t const n_rows);

#endif // MPI_UTIL_H
