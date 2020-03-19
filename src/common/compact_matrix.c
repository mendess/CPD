#include "compact_matrix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// CSR (compressed sparse row)
// [
// [0, 0, 0, 0],
// [5, 8, 0, 0],
// [0, 0, 3, 0],
// [0, 6, 0, 0]
// ]
//
// vals: [  5,8,3,6]
// cols: [  0,1,2,1]
// rows: [0,0,  2,3,4]
//
// row_start = rows[1]; 0
// row_end   = rows[1 + 1]; 2
//
// ROW_VALUES = vals[row_start:row_end]; [5,8]
// ROW_COLS   = cols[0        :2      ]; [0,1]
//
// [
// [0, 5, 0, 0],
// [0, 8, 0, 6],
// [0, 0, 3, 0],
// [0, 0, 0, 0]
// ]
// Changing is hard

CompactMatrix
cmatrix_make(size_t rows, size_t const columns, size_t const num_elems) {
    return (CompactMatrix){.values = calloc(sizeof(double), num_elems),
                           .columns = calloc(sizeof(size_t), num_elems),
                           .rows = calloc(1 + rows, sizeof(size_t)),
                           .n_rows = rows,
                           .n_cols = columns,
                           .n_elems = num_elems};
}

void cmatrix_add(
    CompactMatrix* const m,
    size_t const row,
    size_t const column,
    double const value) {
    size_t const row_end = m->rows[row + 1];
    memmove(
        m->values + row_end + 1,
        m->values + row_end,
        (m->n_elems - row_end - 1) * sizeof(double));
    memmove(
        m->columns + row_end + 1,
        m->columns + row_end,
        (m->n_elems - row_end - 1) * sizeof(size_t));
    size_t const* const end = m->rows + m->n_rows + 1;
    for (size_t* rows = m->rows + row + 1; rows != end; ++rows) { ++(*rows); }
    m->values[row_end] = value;
    m->columns[row_end] = column;
}

CMatrixIter cmatrix_iter(CompactMatrix const* const m) {
    return (CMatrixIter){
        .m = m,
        .column_idx = 0,
        .row_idx = 1,
    };
}

CMatrixIterRow
cmatrix_iter_row(CompactMatrix const* const m, size_t const row) {
    size_t const start_col_idx = m->rows[row];
    size_t const end_col_idx = m->rows[row + 1];
    return (CMatrixIterRow){
        .iter = m->values + start_col_idx,
        .end = m->values + end_col_idx,
        .column = m->columns + start_col_idx,
    };
}

CMatrixIterItem cmatrix_iter_next(CMatrixIter* const iter) {
    while (iter->row_idx == iter->column_idx) { ++iter->row_idx; }
    CMatrixIterItem cmi = (CMatrixIterItem){
        .row = iter->row_idx,
        .column = iter->column_idx,
        .value = iter->m->values + iter->column_idx,
    };
    ++iter->column_idx;
    return cmi;
}

CMatrixIterRowItem cmatrix_iter_row_next(CMatrixIterRow* const iter) {
    CMatrixIterRowItem cmri = (CMatrixIterRowItem){
        .value = iter->iter,
        .column = *iter->column,
    };
    ++iter->iter;
    ++iter->column;
    return cmri;
}

void cmatrix_print(CompactMatrix const* m) {
    for (size_t r = 0; r < m->n_rows; r++) {
        for (size_t c = 0; c < m->n_cols; ++c) {
            fprintf(stderr, "%.6lf ", *cmatrix_at(m, r, c));
        }
        fputc('\n', stderr);
    }
}

void cmatrix_free(CompactMatrix* m) {
    free(m->rows);
    free(m->columns);
    free(m->values);
}
