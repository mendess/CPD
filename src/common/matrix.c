#include "matrix.h"

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAND01 ((double) random() / (double) RAND_MAX)

Matrix matrix_make(size_t rows, size_t columns) {
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

double const* matrix_at(Matrix const* m, size_t row, size_t column) {
    return m->data + (row * m->columns + column);
}

double* matrix_at_mut(Matrix* m, size_t row, size_t column) {
    return m->data + (row * m->columns + column);
}

void matrix_free(Matrix m) {
    free(m.data);
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

MatrixIterMut matrix_iter_full_mut(Matrix* m) {
    return (MatrixIterMut){.iter = m->data,
                           .end = m->data + (m->rows * m->columns)};
}

void matrix_print(Matrix const* m) {
    for (size_t i = 0; i < m->rows; i++) {
        for (size_t j = 0; j < m->columns; j++) {
            printf("%.3lf   ", *matrix_at(m, i, j));
        }
        printf("\n");
    }
    printf("\n");
}

void random_fill_LR(size_t nF, Matrix* l, Matrix* r) {
    srandom(0);

    for (MatrixIterMut i = matrix_iter_full_mut(l); i.iter != i.end; ++i.iter) {
        *i.iter = RAND01 / (double) nF; // Why is this division being done?
    }

    for (MatrixIterMut i = matrix_iter_full_mut(r); i.iter != i.end; ++i.iter) {
        *i.iter = RAND01 / (double) nF;
    }
}
