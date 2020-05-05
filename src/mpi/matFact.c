#include "mpi/matFact.h"

#include "common/compact_matrix.h"
#include "common/parser.h"

#include <assert.h>
#include <stdio.h>

#define DELTA(a, b, lr) (2 * ((a) - (b)) * -(lr))

typedef struct Slice {
    Item* start;
    Item* end;
} Slice;

void matrix_b_full(Matrix const* l, Matrix const* r, Matrix* matrix) {
    assert(l->columns == r->rows);
    for (size_t i = 0; i < l->rows; i++) {
        for (size_t j = 0; j < r->columns; ++j) {
            for (size_t k = 0; k < l->columns; ++k) {
                *MATRIX_AT_MUT(matrix, i, j) +=
                    *MATRIX_AT(l, i, k) * *MATRIX_AT(r, k, j);
            }
        }
    }
}

void matrix_b(
    Matrix const* l, Matrix const* r, Matrix* matrix, CompactMatrix const* a) {
    Item const* iter = a->items;
    Item const* const end = iter + a->current_items;

    while (iter != end) {
        double bij = 0;
        for (size_t k = 0; k < l->columns; k++) {
            bij += *MATRIX_AT(l, iter->row, k) * *MATRIX_AT(r, k, iter->column);
        }
        *MATRIX_AT_MUT(matrix, iter->row, iter->column) = bij;
        ++iter;
    }
}

void next_iter_l(
    Matrices const* matrices, Slice a, Matrix* aux_l, Matrix const* b) {
    (void) a;
    Item const* iter = matrices->a.items;
    Item const* const end = iter + matrices->a.current_items;

    while (iter != end) {
        size_t counter = 0;
        for (size_t k = 0; k < matrices->l.columns; k++) {
            double aux = 0;
            Item const* line_iter = iter;
            size_t const row = line_iter->row;
            counter = 0;
            while (line_iter != end && line_iter->row == row) {
                size_t const column = line_iter->column;
                aux += DELTA(
                    line_iter->value,
                    *MATRIX_AT(b, row, column),
                    *MATRIX_AT(&matrices->r, k, column));
                ++line_iter;
                ++counter;
            }
            // l is transposed
            *MATRIX_AT_MUT(aux_l, row, k) =
                *MATRIX_AT(&matrices->l, row, k) - matrices->alpha * aux;
        }
        iter += counter;
    }
}

void next_iter_r(
    Matrices const* matrices, Slice a, Matrix* aux_r, Matrix const* b) {
    (void) a;
    for (size_t k = 0; k < matrices->r.rows; k++) {
        Item const* iter = matrices->a_transpose.items;
        Item const* const end = iter + matrices->a_transpose.current_items;
        for (size_t column = 0; iter != end && column < matrices->r.columns;
             column++) {
            double aux = 0;
            size_t const column = iter->row;
            while (iter != end && iter->row == column) {
                size_t const row = iter->column;
                aux += DELTA(
                    iter->value,
                    *MATRIX_AT(b, row, column),
                    // l is transposed
                    *MATRIX_AT(&matrices->l, row, k));
                ++iter;
            }
            *MATRIX_AT_MUT(aux_r, k, column) =
                *MATRIX_AT(&matrices->r, k, column) - matrices->alpha * aux;
        }
    }
}

static inline void swap(Matrix* a, Matrix* b) {
    Matrix tmp = *a;
    *a = *b;
    *b = tmp;
}

static inline size_t
start_chunk(size_t const proc_id, int const nprocs, size_t const num_iters) {
    size_t const rem = (num_iters % nprocs);
    size_t const x = proc_id * (num_iters - rem) / nprocs;
    return x + (proc_id < rem ? proc_id : rem);
}

static inline Slice split(CompactMatrix* a, int proc_id, int nprocs) {
    size_t start = start_chunk(proc_id, nprocs, a->n_rows);
    size_t end = start_chunk(proc_id + 1, nprocs, a->n_rows);
    fprintf(stderr, "Node %d starts ends [%zu, %zu[\n", proc_id, start, end);
    Item* start_p = a->items + a->row_pos[start];
    Item* end_p = a->items + a->row_pos[end];
    return (Slice){.start = start_p, .end = end_p};
}

Matrix iter_mpi(Matrices* matrices, int nprocs, int me) {
    (void) nprocs;
    (void) me;
    Matrix aux_l = matrix_clone(&matrices->l);
    Matrix aux_r = matrix_clone(&matrices->r);
    Matrix b = matrix_make(matrices->a.n_rows, matrices->a.n_cols);
    for (size_t i = 0; i < matrices->num_iterations; i++) {
        matrix_b(&matrices->l, &matrices->r, &b, &matrices->a);
        next_iter_l(matrices, (Slice){NULL, NULL}, &aux_l, &b);
        next_iter_r(matrices, (Slice){NULL, NULL}, &aux_r, &b);
        swap(&matrices->l, &aux_l);
        swap(&matrices->r, &aux_r);
    }
    matrix_b_full(&matrices->l, &matrices->r, &b);
    matrix_free(&aux_l);
    matrix_free(&aux_r);
    return b;
}
