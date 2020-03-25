#include "matFact.h"

#include "compact_matrix.h"
#include "parser.h"

#include <assert.h>
#include <stdio.h>

#define DELTA(a, b, lr) (2 * ((a) - (b)) * -(lr))

void matrix_b_full(Matrix const* l, Matrix const* r, Matrix* matrix) {
    assert(l->columns == r->rows);
#pragma omp parallel for
    for (size_t i = 0; i < l->rows; i++) {
        for (size_t j = 0; j < r->columns; ++j) {
            for (size_t k = 0; k < l->columns; ++k) {
                *MATRIX_AT(matrix, i, j) +=
                    *MATRIX_AT(l, i, k) * *MATRIX_AT(r, k, j);
            }
        }
    }
}

void matrix_b(
    Matrix const* l, Matrix const* r, Matrix* matrix, CompactMatrix const* a) {
    Item const* const end = a->items + a->current_items;
#pragma omp parallel for
    for (Item const* iter = a->items; iter != end; ++iter) {
        double bij = 0;
        for (size_t k = 0; k < l->columns; k++) {
            bij += *MATRIX_AT(l, iter->row, k) * *MATRIX_AT(r, k, iter->column);
        }
        *MATRIX_AT(matrix, iter->row, iter->column) = bij;
    }
}

void next_iter_l(Matrices const* matrices, Matrix* aux_l, Matrix const* b) {
    Item const* iter = matrices->a_prime.items;
    Item const* const end = iter + matrices->a_prime.current_items;

    for (size_t row = 0; row < matrices->l.rows; row++) {
        if (iter != end && iter->row == row) {
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
                *MATRIX_AT(aux_l, row, k) =
                    *MATRIX_AT(&matrices->l, row, k) - matrices->alpha * aux;
            }
            iter += counter;
        } else {
            for (size_t k = 0; k < matrices->l.columns; k++) {
                *MATRIX_AT(aux_l, row, k) = *MATRIX_AT(&matrices->l, row, k);
            }
        }
    }
}

void next_iter_r(Matrices const* matrices, Matrix* aux_r, Matrix const* b) {
    for (size_t k = 0; k < matrices->r.rows; k++) {
        Item const* iter = matrices->a_prime_transpose.items;
        Item const* const end =
            iter + matrices->a_prime_transpose.current_items;
        for (size_t column = 0; column < matrices->r.columns; column++) {
            if (iter != end && iter->row == column) {
                double aux = 0;
                size_t const column = iter->row;
                while (iter != end && iter->row == column) {
                    size_t const row = iter->column;
                    aux += DELTA(
                        iter->value,
                        *MATRIX_AT(b, row, column),
                        *MATRIX_AT(&matrices->l, row, k));
                    ++iter;
                }
                *MATRIX_AT(aux_r, k, column) =
                    *MATRIX_AT(&matrices->r, k, column) - matrices->alpha * aux;
            } else {
                *MATRIX_AT(aux_r, k, column) =
                    *MATRIX_AT(&matrices->r, k, column);
            }
        }
    }
}

static inline void swap(Matrix* a, Matrix* b) {
    Matrix tmp = *a;
    *a = *b;
    *b = tmp;
}

Matrix iter(Matrices* matrices) {
    Matrix aux_l = matrix_make(matrices->l.rows, matrices->l.columns);
    Matrix aux_r = matrix_make(matrices->r.rows, matrices->r.columns);
    Matrix b = matrix_make(matrices->a_prime.n_rows, matrices->a_prime.n_cols);
    for (size_t i = 0; i < matrices->num_iterations; i++) {
        matrix_b(&matrices->l, &matrices->r, &b, &matrices->a_prime);
        next_iter_l(matrices, &aux_l, &b);
        next_iter_r(matrices, &aux_r, &b);
        swap(&matrices->l, &aux_l);
        swap(&matrices->r, &aux_r);
    }
    matrix_b_full(&matrices->l, &matrices->r, &b);
    matrix_free(&aux_l);
    matrix_free(&aux_r);
    return b;
}
