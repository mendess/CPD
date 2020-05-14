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
#define RAND01_r(r) ((double) (r) / (double) RAND_MAX)

void random_fill_LT_R_mpi(
    Matrix* const l,
    Matrix* const r,
    int const me,
    int const nprocs,
    size_t const k) {

    srandom(0);

    Slice s = slice_rows(me, nprocs, k);
    for (size_t i = 0; i < l->columns; ++i) {
        for (size_t j = 0; j < k; ++j) {
            if (s.start <= j && j < s.end) {
                size_t real_j = j - s.start;
                *MATRIX_AT_MUT(l, real_j, i) = RAND01 / (double) k;
            } else {
                random();
            }
        }
    }

    for (size_t row = 0; row < s.start; ++row) {
        MatrixIterMut j = matrix_iter_row_mut(r, row);
        for (; j.iter != j.end; ++j.iter) random();
    }
    for (size_t row = s.start; row < s.end; ++row) {
        size_t ki = row - s.start;
        MatrixIterMut j = matrix_iter_row_mut(r, ki);
        for (; j.iter != j.end; ++j.iter) {
            *j.iter = RAND01 / (double) k;
        }
    }
}

void random_fill_L_RT_mpi(
    Matrix* const l,
    Matrix* const r,
    int const me,
    int const nprocs,
    size_t const k) {

    srandom(0);

    Slice s = slice_rows(me, nprocs, k);
    for (size_t row = 0; row < s.start; ++row) {
        MatrixIterMut j = matrix_iter_row_mut(l, row);
        for (; j.iter != j.end; ++j.iter) random();
    }

    for (size_t row = s.start; row < s.end; ++row) {
        size_t ki = row - s.start;
        MatrixIterMut j = matrix_iter_row_mut(l, ki);
        for (; j.iter != j.end; ++j.iter) {
            *j.iter = RAND01 / (double) k;
        }
    }

    for (size_t i = 0; i < r->rows; ++i) {
        for (size_t j = 0; j < k; ++j) {
            if (s.start <= j && j < s.end) {
                size_t real_j = j - s.start;
                *MATRIX_AT_MUT(r, real_j, i) = RAND01 / (double) k;
            } else {
                random();
            }
        }
    }
}
