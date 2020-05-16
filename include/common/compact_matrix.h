#ifndef COMPACT_MATRIX_H
#define COMPACT_MATRIX_H

#include <assert.h>
#include <stddef.h>

typedef struct {
    double value;
    size_t row;
    size_t column;
} Item;

typedef struct {
    Item* items;
    size_t* row_lengths;
    // This field is used to maintain invariants. DO NOT USE PLS K THKS
    size_t _total_items;
    size_t current_items;
    size_t n_rows;
    size_t n_cols;
} CompactMatrix;

CompactMatrix cmatrix_make(size_t rows, size_t columns, size_t num_elems);

CompactMatrix
cmatrix_make_without_lengths(size_t rows, size_t columns, size_t num_elems);

void cmatrix_add(CompactMatrix* m, size_t row, size_t column, double value);

void cmatrix_add_without_lengths(
    CompactMatrix* m, size_t row, size_t column, double value);

void cmatrix_sort(CompactMatrix* m);

void cmatrix_print(CompactMatrix const* m);

void cmatrix_free(CompactMatrix* m);

#endif // COMPACT_MATRIX_H
