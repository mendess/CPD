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
    memcpy(
        new_m.data, other->data, sizeof(double) * new_m.rows * new_m.columns);
    return new_m;
}

double const*
matrix_at(Matrix const* const m, size_t const row, size_t const column) {
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
            printf("%.6lf ", *i.iter);
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

void matrix_clear(Matrix* m) {
    memset(m->data, 0, m->rows * m->columns * sizeof(double));
}

void matrices_free(Matrices* m) {
    matrix_free(&m->a);
    matrix_free(&m->l);
    matrix_free(&m->r);
}

void print_output(Matrices const* const matrices, Matrix const* const b) {
    double max;
    size_t max_pos;
    for (size_t row = 0; row < matrices->a.rows; row++) {
        max = 0;
        max_pos = 0;
        for (size_t column = 0; column < matrices->a.columns; column++) {
            if (*matrix_at(&(matrices->a), row, column) == 0) {
                double aux = *matrix_at(b, row, column);
                if (aux > max) {
                    max = aux;
                    max_pos = column;
                }
            }
        }
        printf("%ld\n", max_pos);
    }
}