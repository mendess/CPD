#ifndef MATRIX_H
#define MATRIX_H
#ifndef _DEFAULT_SOURCE
#    define _DEFAULT_SOURCE
#endif
#include "common/compact_matrix.h"

#include <assert.h>
#include <stddef.h>
#ifndef NO_ASSERT
#    include <stdio.h>
#endif

typedef struct {
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

void random_fill_L_RT(Matrix* const l, Matrix* const r);

void random_fill_LR(Matrix* l, Matrix* r);

typedef struct {
    Matrix m;
    size_t row_offset;
    size_t column_offset;
} VMatrix;

VMatrix vmatrix_make(
    size_t start_row, size_t end_row, size_t start_column, size_t end_column);

VMatrix vmatrix_clone(VMatrix const* other);

#ifdef NO_ASSERT
#    define VMATRIX_AT(m, row, column) \
        (MATRIX_AT(                    \
            (&(m)->m), (row) - (m)->row_offset, column - (m)->column_offset))
#    define VMATRIX_AT_MUT(m, row, column) \
        (MATRIX_AT_MUT(                    \
            (&(m)->m), (row) - (m)->row_offset, column - (m)->column_offset))
#else
#    define VMATRIX_AT(m, row, column) (vmatrix_at((m), (row), (column)))
#    define VMATRIX_AT_MUT(m, row, column) \
        (vmatrix_at_mut((m), (row), (column)))

double const* vmatrix_at(VMatrix const* a, size_t row, size_t column);

double* vmatrix_at_mut(VMatrix* a, size_t row, size_t column);
#endif

void vmatrix_print(VMatrix const* m, size_t rows, size_t columns);

void vmatrix_print_with_name(
    VMatrix const* m, char const* name, size_t rows, size_t columns);

void vmatrix_free(VMatrix* m);

typedef struct {
    size_t num_iterations;
    double alpha;
    CompactMatrix a;
    CompactMatrix a_transpose;
    Matrix l;
    Matrix r;
} Matrices;

typedef struct {
    size_t num_iterations;
    double alpha;
    CompactMatrix a;
    CompactMatrix a_transpose;
    VMatrix l;
    VMatrix r;
} VMatrices;

void matrices_free(Matrices* m);

void vmatrices_free(VMatrices* m);

void print_output(CompactMatrix const* const matrices, Matrix const* const b);

#endif // MATRIX_H
