#ifndef PARSER_H
#define PARSER_H
#include <stddef.h>

typedef struct Matrix {
    size_t rows;
    size_t columns;
    double* data;
} Matrix;

double const* matrix_at(Matrix const* m, size_t row, size_t column);

double* matrix_at_mut(Matrix* m, size_t row, size_t column);

void matrix_print(Matrix const* m);

typedef struct MatrixIter {
    double* iter;
    double* end;
} MatrixIter;

MatrixIter matrix_iter_row(Matrix* m, size_t row);

MatrixIter matrix_iter_full(Matrix* m);

typedef struct Matrixes {
    size_t num_iterations;
    float alpha;
    Matrix a;
    Matrix l;
    Matrix r;
} Matrixes;

typedef enum ParserError {
    PARSER_ERROR_OK = 0,
    PARSER_ERROR_IO,
    PARSER_ERROR_INVALID_FORMAT,
} ParserError;

ParserError parse_file(char const* filename, Matrixes* matrixes);

#endif // PARSER_H
