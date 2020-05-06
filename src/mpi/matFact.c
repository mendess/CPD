#include "mpi/matFact.h"

#include "common/compact_matrix.h"
#include "common/matrix.h"
#include "common/parser.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define eprintf(...) (fprintf(stderr, ...))

#define DELTA(a, b, lr) (2 * ((a) - (b)) * -(lr))

typedef struct {
    Item const* start;
    Item const* const end;
} Slice;

void matrix_b_full(Matrix const* l, Matrix const* r, Matrix* b) {
    assert(l->rows == r->rows);
    for (size_t i = 0; i < l->columns; i++) {
        for (size_t j = 0; j < r->columns; ++j) {
            for (size_t k = 0; k < l->rows; ++k) {
                *MATRIX_AT_MUT(b, i, j) +=
                    *MATRIX_AT(l, k, i) * *MATRIX_AT(r, k, j);
            }
        }
    }
}

void matrix_b(Matrix const* l, Matrix const* r, Matrix* b, Slice a) {
    Item const* iter = a.start;
    Item const* const end = a.end;

    while (iter != end) {
        double bij = 0;
        for (size_t k = 0; k < l->rows; k++) {
            bij += *MATRIX_AT(l, k, iter->row) * *MATRIX_AT(r, k, iter->column);
        }
        *MATRIX_AT_MUT(b, iter->row, iter->column) = bij;
        ++iter;
    }
}

// This method is only define for b's of the same size
//
void matrix_b_add(Matrix* b, Matrix const* other_b) {
    double* iter = b->data;
    double const* other_iter = other_b->data;
    double const* const end = iter + (b->rows * b->columns);
    while (iter != end) {
        *iter += *other_iter;
        ++iter;
        ++other_iter;
    }
}

void next_iter_l(
    Matrices const* matrices,
    Slice at,
    size_t k_start,
    size_t k_end,
    Matrix* aux_l,
    Matrix const* b) {
    for (size_t k = k_start; k < k_end; ++k) {
        size_t counter = 0;
        Item const* iter = at.start;
        Item const* const end = at.end;
        while (iter != end) {
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
            *MATRIX_AT_MUT(aux_l, k, row) =
                *MATRIX_AT(&matrices->l, k, row) - matrices->alpha * aux;
            iter += counter;
        }
    }
}

void next_iter_r(
    Matrices const* matrices,
    Slice a,
    size_t k_start,
    size_t k_end,
    Matrix* aux_r,
    Matrix const* b) {
    for (size_t k = k_start; k < k_end; ++k) {
        Item const* iter = a.start;
        Item const* const end = a.end;
        for (size_t column = 0; iter != end && column < matrices->r.columns;
             column++) {
            double aux = 0;
            size_t const column = iter->row;
            while (iter != end && iter->row == column) {
                size_t const row = iter->column;
                aux += DELTA(
                    iter->value,
                    *MATRIX_AT(b, row, column),
                    *MATRIX_AT(&matrices->l, k, row));
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
    Matrix aux_l = matrix_clone(&matrices->l);
    Matrix aux_r = matrix_clone(&matrices->r);
    Matrix b = matrix_make(matrices->a.n_rows, matrices->a.n_cols);
    Slice a = {
        matrices->a.items, matrices->a.items + matrices->a.current_items};
    Slice at = {
        matrices->a_transpose.items,
        matrices->a_transpose.items + matrices->a_transpose.current_items};
    for (size_t i = 0; i < matrices->num_iterations; i++) {
        matrix_b(&matrices->l, &matrices->r, &b, a);
        next_iter_l(matrices, a, 0, aux_l.rows, &aux_l, &b);
        next_iter_r(matrices, at, 0, aux_r.rows, &aux_r, &b);
        swap(&matrices->l, &aux_l);
        swap(&matrices->r, &aux_r);
    }
    matrix_b_full(&matrices->l, &matrices->r, &b);
    matrix_free(&aux_l);
    matrix_free(&aux_r);
    return b;
}
