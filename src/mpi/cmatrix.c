#ifndef _DEFAULT_SOURCE
#    define _DEFAULT_SOURCE
#endif
#include "common/compact_matrix.h"
#include "common/debug.h"
#include "common/matrix.h"
#include "mpi/util.h"

#include <mpi.h>
#include <stdlib.h>

#define RAND01 ((double) random() / (double) RAND_MAX)

void random_fill_LR_parts(
    Matrix* const l, Matrix* const r, CompactMatrix const* const a) {

    ABounds abounds = a_bounds(ME, a->n_rows, a->n_cols);

    size_t row;
    for (row = 0; row < abounds.i.start; ++row) {
        for (size_t col = 0; col < l->columns; ++col) random();
    }

    for (; row < abounds.i.end; ++row) {
        size_t ki = row - abounds.i.start;
        MatrixIterMut j = matrix_iter_row_mut(l, ki);
        for (; j.iter != j.end; ++j.iter) {
            *j.iter = RAND01 / (double) l->columns;
        }
    }

    for (; row < a->n_rows; ++row) {
        for (size_t col = 0; col < l->columns; ++col) random();
    }

    for (row = 0; row < r->rows; ++row) {
        size_t col = 0;
        for(; col < abounds.j.start; ++col) random();
        for(; col < abounds.j.end; ++col) {
            *MATRIX_AT_MUT(r, row, col - abounds.j.start) = RAND01 / (double) r->rows;
        }
        for(; col < a->n_cols; ++col) random();
    }
}
