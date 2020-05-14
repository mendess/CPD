#ifndef MPI_UTIL_H
#define MPI_UTIL_H
#include "common/matrix.h"

#include <stddef.h>

extern int NPROCS;
extern int ME;
extern int CHECKER_BOARD_SIDE;

typedef struct {
    size_t start;
    size_t end;
} Slice;

void swap(Matrix* a, Matrix* b);

size_t start_chunk(int proc_id, int nprocs, size_t num_iters);

int proc_from_chunk(size_t k, int nprocs, size_t num_iters);

Slice slice_rows(int proc_id, int nprocs, size_t n_rows);

size_t slice_len(int proc_id, int nprocs, size_t n_rows);

#endif // MPI_UTIL_H
