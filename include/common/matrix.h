#ifndef MATRIX_H
#define MATRIX_H
#include <stddef.h>

typedef struct Matrix {
    size_t rows;
    size_t columns;
    double* data;
} Matrix;

Matrix matrix_make(size_t rows, size_t columns);

double const* matrix_at(Matrix const* m, size_t row, size_t column);

double* matrix_at_mut(Matrix* m, size_t row, size_t column);

void matrix_print(Matrix const* m);

void matrix_free(Matrix m);

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

#endif // MATRIX_H
