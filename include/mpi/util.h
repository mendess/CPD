#ifndef MPI_UTIL_H
#define MPI_UTIL_H
#include "common/matrix.h"

#include <stddef.h>
#include <stdbool.h>

extern unsigned NPROCS;
extern unsigned ME;
extern unsigned CHECKER_BOARD_SIDE;

typedef struct {
    size_t start;
    size_t end;
} Slice;

void swap(Matrix* a, Matrix* b);

void vswap(VMatrix* a, VMatrix* b);

bool should_work_alone(size_t rows, size_t columns);

Slice slice_rows(unsigned proc_id, unsigned nprocs, size_t n_rows);

size_t slice_len(unsigned proc_id, unsigned nprocs, size_t n_rows);

typedef struct {
    Slice i;
    Slice j;
} ABounds;

ABounds a_bounds(unsigned zone, size_t rows, size_t columns);

unsigned proc_from_row_column(
    size_t row, size_t column, size_t rows, size_t columns);

#endif // MPI_UTIL_H
