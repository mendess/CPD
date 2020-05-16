#include "common/compact_matrix.h"

#include "common/debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// CSR (compressed sparse row)
CompactMatrix
cmatrix_make(size_t rows, size_t const columns, size_t const num_elems) {
    CompactMatrix c = (CompactMatrix){
        .items = malloc(sizeof(Item) * num_elems),
        .row_lengths = calloc(sizeof(size_t), rows),
        ._total_items = num_elems,
        .current_items = 0,
        .n_rows = rows,
        .n_cols = columns};
    return c;
}

CompactMatrix cmatrix_make_without_lengths(
    size_t rows, size_t const columns, size_t const num_elems) {
    CompactMatrix c = (CompactMatrix){
        .items = malloc(sizeof(Item) * num_elems),
        .row_lengths = NULL,
        ._total_items = num_elems,
        .current_items = 0,
        .n_rows = rows,
        .n_cols = columns};
    return c;
}

void cmatrix_add(
    CompactMatrix* const m,
    size_t const row,
    size_t const column,
    double const value) {
    assert(m->current_items < m->_total_items);
    ++m->row_lengths[row];
    m->items[m->current_items++] =
        (Item){.value = value, .row = row, .column = column};
}

void cmatrix_add_without_lengths(
    CompactMatrix* const m,
    size_t const row,
    size_t const column,
    double const value) {
    assert(m->current_items < m->_total_items);
    m->items[m->current_items++] =
        (Item){.value = value, .row = row, .column = column};
}

void cmatrix_print(CompactMatrix const* m) {
    Item const* iter = m->items;
    Item const* const end = iter + m->current_items;
    eprintln("#current_items = %zu", m->current_items);
    eprintln("(rows, columns) = (%zu, %zu)", m->n_rows, m->n_cols);
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
    assert(iter == end);
}

void cmatrix_print_items(CompactMatrix const* const m) {
    Item const* iter = m->items;
    Item const* const end = iter + m->current_items;
    while (iter != end) {
        eprintln(
            "i{row: %zu, column: %zu, value: %f}",
            iter->row,
            iter->column,
            iter->value);
        ++iter;
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
