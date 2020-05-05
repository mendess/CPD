#include "common/compact_matrix.h"

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
    return (CompactMatrix){
        .items = malloc(sizeof(Item) * num_elems),
        .row_lengths = calloc(sizeof(size_t), rows),
        ._total_items = num_elems,
        .row_pos = calloc(sizeof(size_t), (rows + 1)),
        .current_items = 0,
        .n_rows = rows,
        .n_cols = columns};
}

void cmatrix_add(
    CompactMatrix* const m,
    size_t row,
    size_t const column,
    double const value) {
    assert(m->current_items < m->_total_items);
    ++m->row_lengths[row];
    m->items[m->current_items] =
        (Item){.value = value, .row = row, .column = column};
    if (m->current_items > 0 &&
        m->items[m->current_items].row != m->items[m->current_items - 1].row) {
        m->row_pos[row] = m->current_items;
        if (row > 1 && m->row_pos[row - 1] == 0) {
            row = row - 1;
            while (m->row_lengths[row] == 0) {
                if (m->row_pos[row] == 0) {
                    m->row_pos[row] = m->current_items;
                } else {
                    break;
                }
                row--;
            }
        }
    }
    m->current_items++;
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
    Item const* a_ = a;
    Item const* b_ = b;
    if (a_->row < b_->row) {
        return -1;
    } else if (a_->row > b_->row) {
        return 1;
    } else if (a_->column < b_->column) {
        return -1;
    } else if (a_->column > b_->column) {
        return 1;
    } else {
        return 0;
    }
}

void cmatrix_sort(CompactMatrix* m) {
    qsort(m->items, m->current_items, sizeof(Item), item_compare);
}

void cmatrix_free(CompactMatrix* m) {
    free(m->items);
    free(m->row_lengths);
}
