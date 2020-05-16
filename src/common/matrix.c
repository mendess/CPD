#include "common/matrix.h"

#include "common/debug.h"

#include <assert.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

#define RAND01 ((double) random() / (double) RAND_MAX)

Matrix matrix_make(size_t const rows, size_t const columns) {
    return (Matrix){
        .rows = rows,
        .columns = columns,
        .data = calloc(rows * columns, sizeof(double))};
}

Matrix matrix_clone(Matrix const* const other) {
    Matrix new_m = (Matrix){
        .rows = other->rows,
        .columns = other->columns,
        .data = malloc(other->rows * other->columns * sizeof(double)),
    };
    memcpy(
        new_m.data, other->data, sizeof(double) * new_m.rows * new_m.columns);
    return new_m;
}

noreturn static void
out_of_bounds(size_t rows, size_t columns, size_t row, size_t column) {
    eprintf(
        "Bounds were (%zu, %zu) but index is (%zu, %zu)\n",
        rows,
        columns,
        row,
        column);
    debug_print_backtrace("access out of bounds");
}

double const* matrix_at(Matrix const* a, size_t row, size_t column) {
    if (row >= a->rows || column >= a->columns) {
        out_of_bounds(a->rows, a->columns, row, column);
    }
    return a->data + (row * a->columns + column);
}

double* matrix_at_mut(Matrix* a, size_t row, size_t column) {
    if (row >= a->rows || column >= a->columns) {
        out_of_bounds(a->rows, a->columns, row, column);
    }
    return a->data + (row * a->columns + column);
}

void matrix_free(Matrix* m) {
    free(m->data);
}

MatrixIter matrix_iter_row(Matrix const* const m, size_t const row) {
    return (MatrixIter){
        .iter = m->data + (row * m->columns),
        .end = m->data + (row * m->columns + m->columns),
    };
}

MatrixIter matrix_iter_full(Matrix const* const m) {
    return (MatrixIter){
        .iter = m->data, .end = m->data + (m->rows * m->columns)};
}

MatrixIterMut matrix_iter_row_mut(Matrix* const m, size_t const row) {
    return (MatrixIterMut){
        .iter = m->data + (row * m->columns),
        .end = m->data + (row * m->columns + m->columns),
    };
}

MatrixIterMut matrix_iter_full_mut(Matrix* const m) {
    return (MatrixIterMut){
        .iter = m->data, .end = m->data + (m->rows * m->columns)};
}

void matrix_print_with_name(Matrix const* const m, char const* const name) {
    eprintf("%s =\n", name);
    for (size_t r = 0; r < m->rows; r++) {
        for (MatrixIter i = matrix_iter_row(m, r); i.iter != i.end; ++i.iter) {
            fprintf(stderr, "%.6lf ", *i.iter);
        }
        fputc('\n', stderr);
    }
}

void matrix_print(Matrix const* const m) {
    matrix_print_with_name(m, "Matrix");
}

void random_fill_LT_R(Matrix* const l, Matrix* const r) {
    srandom(0);

    for (size_t i = 0; i < l->columns; ++i) {
        for (size_t j = 0; j < l->rows; ++j) {
            *MATRIX_AT_MUT(l, j, i) = RAND01 / (double) r->rows;
        }
    }

    for (MatrixIterMut i = matrix_iter_full_mut(r); i.iter != i.end; ++i.iter) {
        *i.iter = RAND01 / (double) r->rows;
    }
}

void random_fill_L_RT(Matrix* const l, Matrix* const r) {
    srandom(0);

    for (MatrixIterMut i = matrix_iter_full_mut(l); i.iter != i.end; ++i.iter) {
        *i.iter = RAND01 / (double) r->rows;
    }

    for (size_t i = 0; i < r->columns; ++i) {
        for (size_t j = 0; j < r->rows; ++j) {
            *MATRIX_AT_MUT(r, j, i) = RAND01 / (double) r->rows;
        }
    }
}

void random_fill_LR(Matrix* const l, Matrix* const r) {
    srandom(0);

    for (MatrixIterMut i = matrix_iter_full_mut(l); i.iter != i.end; ++i.iter) {
        *i.iter = RAND01 / (double) r->rows; // Why is this division being done?
    }

    for (MatrixIterMut i = matrix_iter_full_mut(r); i.iter != i.end; ++i.iter) {
        *i.iter = RAND01 / (double) r->rows;
    }
}

VMatrix vmatrix_make(
    size_t const start_row,
    size_t const end_row,
    size_t const start_column,
    size_t const end_column) {

    return (VMatrix){
        .m = matrix_make(end_row - start_row, end_column - start_column),
        .row_offset = start_row,
        .column_offset = start_column};
}

VMatrix vmatrix_clone(VMatrix const* const m) {
    return (VMatrix){
        .m = matrix_clone(&m->m),
        .row_offset = m->row_offset,
        .column_offset = m->column_offset};
}

void vmatrix_change_offsets(
    VMatrix* const m,
    size_t const start_row,
    size_t const end_row,
    size_t const start_column,
    size_t const end_column) {

    m->m.rows = end_row - start_row;
    m->m.columns = end_column - start_column;
    m->row_offset = start_row;
    m->column_offset = start_column;
}

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)

noreturn static void bellow_bounds(
    size_t const row, size_t row_offset, size_t column, size_t column_offset) {

    size_t r = MAX(row_offset, row) - MIN(row_offset, row);
    size_t c = MAX(column_offset, column) - MIN(column_offset, column);
    eprintln(
        "Give the offsets (%zu, %zu) and the indices (%zu, %zu),"
        " indexing was at (%s%zu, %s%zu)",
        row_offset,
        column_offset,
        row,
        column,
        row < row_offset ? "-" : "",
        r,
        column < column_offset ? "-" : "",
        c);
    debug_print_backtrace("indexing bellow bounds");
}

double const* vmatrix_at(VMatrix const* m, size_t row, size_t column) {
    if (row < m->row_offset || column < m->column_offset) {
        bellow_bounds(row, m->row_offset, column, m->column_offset);
    }
    return matrix_at(&m->m, row - m->row_offset, column - m->column_offset);
}

double* vmatrix_at_mut(VMatrix* m, size_t row, size_t column) {
    if (row < m->row_offset || column < m->column_offset) {
        bellow_bounds(row, m->row_offset, column, m->column_offset);
    }
    return matrix_at_mut(&m->m, row - m->row_offset, column - m->column_offset);
}

size_t vmatrix_rows(VMatrix const* const m) {
    return m->m.rows + m->row_offset;
}

size_t vmatrix_cols(VMatrix const* const m) {
    return m->m.columns + m->column_offset;
}

void vmatrix_print_with_name(
    VMatrix const* const m,
    char const* const name,
    size_t rows,
    size_t columns) {
    eprintf("%s =\n", name);
    size_t r = 0;
    rows = MAX(rows, m->m.rows);
    columns = MAX(columns, m->m.columns);
    for (; r < m->row_offset; r++) {
        for (size_t c = 0; c < columns; c++) {
            fprintf(stderr, "\033[2mx.xxxxxx\033[0m ");
        }
        fputc('\n', stderr);
    }
    for (; r < m->row_offset + m->m.rows; r++) {
        size_t c = 0;
        for (; c < m->column_offset; c++) {
            fprintf(stderr, "\033[2mx.xxxxxx\033[0m ");
        }
        for (; c < m->column_offset + m->m.columns; ++c) {
            fprintf(stderr, "%.6lf ", *VMATRIX_AT(m, r, c));
        }
        for (; c < columns; c++) {
            fprintf(stderr, "\033[2mx.xxxxxx\033[0m ");
        }
        fputc('\n', stderr);
    }
    for (; r < rows; ++r) {
        for (size_t c = 0; c < columns; c++) {
            fprintf(stderr, "\033[2mx.xxxxxx\033[0m ");
        }
        fputc('\n', stderr);
    }
}

void vmatrix_print(
    VMatrix const* const m, size_t const rows, size_t const columns) {
    vmatrix_print_with_name(m, "VMatrix", rows, columns);
}

void vmatrix_free(VMatrix* m) {
    matrix_free(&m->m);
}

void matrices_free(Matrices* m) {
    cmatrix_free(&m->a);
    cmatrix_free(&m->a_transpose);
    matrix_free(&m->l);
    matrix_free(&m->r);
}

void vmatrices_free(VMatrices* m) {
    cmatrix_free(&m->a);
    cmatrix_free(&m->a_transpose);
    vmatrix_free(&m->l);
    vmatrix_free(&m->r);
}

void print_output(CompactMatrix const* const a, Matrix const* const b) {
    Item const* iter = a->items;
    Item const* const end = iter + a->current_items;
    for (size_t row = 0; row < a->n_rows; row++) {
        double max = 0;
        size_t max_pos = 0;
        for (size_t column = 0; column < a->n_cols; column++) {
            if (iter != end && iter->row == row && iter->column == column) {
                ++iter;
            } else {
                double aux = *MATRIX_AT(b, row, column);
                /* if (aux != 0.0) */
                /*     eprintln( */
                /*         "(%zu, %zu): Tesing: %f < %f", row, column, aux, max); */
                if (aux > max) {
                    max = aux;
                    max_pos = column;
                }
            }
        }
        printf("%zu\n", max_pos);
    }
}
