#ifndef MATRIX_H
#define MATRIX_H
#define _DEFAULT_SOURCE
#include "common/compact_matrix.h"

#include <assert.h>
#include <stddef.h>
#ifndef NO_ASSERT
#    include <stdio.h>
#endif

typedef struct Matrix {
    size_t rows;
    size_t columns;
    double* data;
} Matrix;

Matrix matrix_make(size_t rows, size_t columns);

Matrix matrix_clone(Matrix const* other);

#ifdef NO_ASSERT
#    define MATRIX_AT(m, row, column) \
        ((m)->data + ((row) * (m)->columns + (column)))
#    define MATRIX_AT_MUT(m, row, column) \
        ((m)->data + ((row) * (m)->columns + (column)))
#else
#    define MATRIX_AT(m, row, column) (matrix_at((m), (row), (column)))
#    define MATRIX_AT_MUT(m, row, column) (matrix_at_mut((m), (row), (column)))

double const* matrix_at(Matrix const* a, size_t row, size_t column);
double* matrix_at_mut(Matrix* a, size_t row, size_t column);
#endif

void matrix_print(Matrix const* m);
void matrix_print_with_name(Matrix const* m, char const* name);

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

void random_fill_LT_R(Matrix* const l, Matrix* const r);

void random_fill_LR(Matrix* l, Matrix* r);

void matrix_clear(Matrix* m);

typedef struct Matrices {
    size_t num_iterations;
    double alpha;
    CompactMatrix a;
    CompactMatrix a_transpose;
    Matrix l;
    Matrix r;
} Matrices;

void matrices_free(Matrices* m);

void print_output(Matrices const* const matrices, Matrix const* const b);

#endif // MATRIX_H
