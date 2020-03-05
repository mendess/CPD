#include "matrix.h"
#include <stdlib.h>
#include <stdio.h>

#define RAND01 ((double) random() / (double) RAND_MAX)

Matrix matrix_make(size_t rows, size_t columns) {
    return (Matrix){.rows = rows,
                    .columns = columns,
                    .data = malloc(rows * columns * sizeof(double))};
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

MatrixIter matrix_iter_row(Matrix* m, size_t row) {
    return (MatrixIter){
        .iter = m->data + (row * m->columns),
        .end = m->data + (row * m->columns + m->columns),
    };
}

MatrixIter matrix_iter_full(Matrix* m) {
    return (MatrixIter){
        .iter = m->data + (0 * m->columns),
        .end = m->data + ((m->rows - 1) * m->columns + m->columns),
    };
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

    for (MatrixIter i = matrix_iter_full(l); i.iter != i.end; ++i.iter) {
        *i.iter = RAND01 / (double) nF; // Why is this division being done?
    }

    for (MatrixIter i = matrix_iter_full(r); i.iter != i.end; ++i.iter) {
        *i.iter = RAND01 / (double) nF;
    }
}

