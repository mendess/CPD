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
    return (CompactMatrix){.items = malloc(sizeof(Item) * num_elems),
                           .total_items = num_elems,
                           .current_items = 0,
                           .n_rows = rows,
                           .n_cols = columns};
}

void cmatrix_add(
    CompactMatrix* const m,
    size_t const row,
    size_t const column,
    double const value) {
    assert(m->current_items < m->total_items);
    m->items[m->current_items++] =
        (Item){.value = value, .row = row, .column = column};
}

void cmatrix_print(CompactMatrix const* m) {
    Item const* iter = m->items;
    Item const* const end = iter + m->current_items;
    for (size_t row = 0; row < m->n_rows; row++) {
        for (size_t column = 0; column < m->n_cols; column++) {
            if (iter != end && iter->row == row && iter->column == column) {
                fprintf(stderr, "%.6lf ", iter->value);
                ++iter;
            } else {
                fprintf(stderr, "%.6lf ", 0.0);
            }
        }
        fputc('\n', stderr);
    }
}

static int item_compare(void const* a, void const* b) {
    Item* a_ = (Item*) a;
    Item* b_ = (Item*) b;
    int c = a_->row - b_->row;
    if (c == 0) { return a_->column - b_->column; }
    return c;
}

void cmatrix_sort(CompactMatrix* m) {
    qsort(m->items, m->current_items, sizeof(Item), item_compare);
}

void cmatrix_free(CompactMatrix* m) {
    free(m->items);
}
