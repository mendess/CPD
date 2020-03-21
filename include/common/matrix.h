#ifndef MATRIX_H
#define MATRIX_H
#define _DEFAULT_SOURCE
#include "compact_matrix.h"

#include <assert.h>
#include <stddef.h>

typedef struct Matrix {
    size_t rows;
    size_t columns;
    double* data;
} Matrix;

Matrix matrix_make(size_t rows, size_t columns);

Matrix matrix_clone(Matrix const* other);

static inline double const*
matrix_at(Matrix const* const m, size_t const row, size_t const column) {
    assert(m->rows > row);
    assert(m->columns > column);
    return m->data + (row * m->columns + column);
}

static inline double*
matrix_at_mut(Matrix* const m, size_t const row, size_t const column) {
    assert(m->rows > row);
    assert(m->columns > column);
    return m->data + (row * m->columns + column);
}

void matrix_print(Matrix const* m);

void matrix_free(Matrix* m);

typedef struct MatrixIter {
    double const* iter;
    double const* const end;
} MatrixIter;

MatrixIter matrix_iter_row(Matrix const* m, size_t row);

MatrixIter matrix_iter_full(Matrix const* m);

typedef struct MatrixIterMut {
    double* iter;
    double const* const end;
} MatrixIterMut;

MatrixIterMut matrix_iter_row_mut(Matrix* m, size_t row);

MatrixIterMut matrix_iter_full_mut(Matrix* m);

void random_fill_LR(size_t nF, Matrix* l, Matrix* r);

void matrix_clear(Matrix* m);

typedef struct Matrices {
    size_t num_iterations;
    double alpha;
    CompactMatrix a_prime;
    CompactMatrix a_prime_transpose;
    Matrix l;
    Matrix r;
} Matrices;

void matrices_free(Matrices* m);

void print_output(Matrices const* const matrices, Matrix const* const b);

#endif // MATRIX_H
