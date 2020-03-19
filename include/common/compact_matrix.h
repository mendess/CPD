#ifndef COMPACT_MATRIX_H
#define COMPACT_MATRIX_H

#include <assert.h>
#include <stddef.h>

typedef struct {
    double* values;
    size_t* columns;
    size_t* rows;
    size_t n_rows;
    size_t n_cols;
    size_t n_elems;
} CompactMatrix;

CompactMatrix cmatrix_make(size_t rows, size_t columns, size_t num_elems);

static inline double const* cmatrix_at(
    CompactMatrix const* const m, size_t const row, size_t const column) {
    assert(m->n_rows > row && m->n_cols > column);
    size_t const start_col_idx = m->rows[row];
    size_t const end_col_idx = m->rows[row + 1];
    static const double ZERO = 0.0;

    for (size_t i = start_col_idx; i < end_col_idx; ++i) {
        if (m->columns[i] == column) { // TODO: optimize if ordered
            return m->values + i;
        }
    }
    return &ZERO;
}

void cmatrix_add(CompactMatrix* m, size_t row, size_t column, double value);

typedef struct {
    CompactMatrix const* const m;
    size_t column_idx;
    size_t row_idx;
} CMatrixIter;

CMatrixIter cmatrix_iter(CompactMatrix const* const m);

typedef struct {
    double const* value;
    size_t row;
    size_t column;
} CMatrixIterItem;

CMatrixIterItem cmatrix_iter_next(CMatrixIter* const iter);

typedef struct {
    double* iter;
    double const* const end;
    size_t* column;
} CMatrixIterRow;

CMatrixIterRow cmatrix_iter_row(CompactMatrix const* const m, size_t row);

typedef struct {
    double const* value;
    size_t column;
} CMatrixIterRowItem;

CMatrixIterRowItem cmatrix_iter_row_next(CMatrixIterRow* const iter);

void cmatrix_print(CompactMatrix const* m);

void cmatrix_free(CompactMatrix* m);

#endif // COMPACT_MATRIX_H
