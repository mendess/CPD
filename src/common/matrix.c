#include "matrix.h"

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAND01 ((double) random() / (double) RAND_MAX)

Matrix matrix_make(size_t const rows, size_t const columns) {
    return (Matrix){.rows = rows,
                    .columns = columns,
                    .data = calloc(rows * columns, sizeof(double))};
}

Matrix matrix_clone(Matrix const* const other) {
    Matrix new_m = (Matrix){
        .rows = other->rows,
        .columns = other->columns,
        .data = malloc(other->rows * other->columns * sizeof(double)),
    };
    memcpy(new_m.data, other->data, new_m.rows * new_m.columns);
    return new_m;
}

double const*matrix_at(Matrix const* const m, size_t const row, size_t const column) {
    return m->data + (row * m->columns + column);
}

double* matrix_at_mut(Matrix* const m, size_t const row, size_t const column) {
    return m->data + (row * m->columns + column);
}

void matrix_free(Matrix* m) {
    free(m->data);
}

MatrixIter matrix_iter_row(Matrix const* const m, size_t const row) {
    return (MatrixIter){
        .iter = m->data + (row * m->columns),
        .end = m->data + (row * m->columns + m->columns),
    };
}

MatrixIter matrix_iter_full(Matrix const* const m) {
    return (MatrixIter){.iter = m->data,
                        .end = m->data + (m->rows * m->columns)};
}

MatrixIterMut matrix_iter_row_mut(Matrix* const m, size_t const row) {
    return (MatrixIterMut){
        .iter = m->data + (row * m->columns),
        .end = m->data + (row * m->columns + m->columns),
    };
}

MatrixIterMut matrix_iter_full_mut(Matrix* const m) {
    return (MatrixIterMut){.iter = m->data,
                           .end = m->data + (m->rows * m->columns)};
}

void matrix_print(Matrix const* m) {
    for (size_t r = 0; r < m->rows; r++) {
        for (MatrixIter i = matrix_iter_row(m, r); i.iter != i.end; ++i.iter) {
            printf("%.6lf   ", *i.iter);
        }
        putchar('\n');
    }
}

void random_fill_LR(size_t const nF, Matrix* const l, Matrix* const r) {
    srandom(0);

    for (MatrixIterMut i = matrix_iter_full_mut(l); i.iter != i.end; ++i.iter) {
        *i.iter = RAND01 / (double) nF; // Why is this division being done?
    }

    for (MatrixIterMut i = matrix_iter_full_mut(r); i.iter != i.end; ++i.iter) {
        *i.iter = RAND01 / (double) nF;
    }
}

Matrix matrix_b(Matrix const *L, Matrix const *R){
    Matrix matrix = matrix_make(L->rows, R->columns);
    for (size_t i = 0; i < L->rows; i++) {
        for (size_t j = 0; j < R->columns; ++j) {
            for (size_t k = 0; k < L->columns; ++k) {
                *matrix_at_mut(&matrix,i,j) += *matrix_at(L, i, k) * *matrix_at(R, k, j);
            }
        }
    }

    //matrix_print(&matrix);
    return matrix;
}
