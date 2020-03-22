#include "matFact.h"

#include "compact_matrix.h"
#include "parser.h"

#include <assert.h>
#include <stdio.h>

static inline double delta(double const a, double const b, double const lr) {
    return 2 * (a - b) * (-lr);
}

void matrix_b(Matrix const* l, Matrix const* r, Matrix* matrix) {
    assert(l->columns == r->rows);
    for (size_t i = 0; i < l->rows; i++) {
        for (size_t j = 0; j < r->columns; ++j) {
            for (size_t k = 0; k < l->columns; ++k) {
                *matrix_at_mut(matrix, i, j) +=
                    *matrix_at(l, i, k) * *matrix_at(r, k, j);
            }
        }
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
                    /* fprintf( */
                    /*     stderr, "(row,column,k) = %zu %zu %zu\n", row,
                     * column, k); */
                    aux += delta(
                        line_iter->value,
                        *matrix_at(b, row, column),
                        *matrix_at(&matrices->r, k, column));
                    ++line_iter;
                    ++counter;
                }
                *matrix_at_mut(aux_l, row, k) =
                    *matrix_at(&matrices->l, row, k) - matrices->alpha * aux;
            }
            iter += counter;
        } else {
            for (size_t k = 0; k < matrices->l.columns; k++) {
                *matrix_at_mut(aux_l, row, k) =
                    *matrix_at(&matrices->l, row, k);
            }
        }
    }
}

void next_iter_r(Matrices const* matrices, Matrix* aux_r, Matrix const* b) {
    for (size_t k = 0; k < matrices->r.rows; k++) {
        Item const* iter = matrices->a_prime_transpose.items;
        Item const* const end =
            iter + matrices->a_prime_transpose.current_items;
        while (iter != end) {
            double aux = 0;
            size_t const column = iter->row;
            while (iter != end && iter->row == column) {
                size_t const row = iter->column;
                aux += delta(
                    iter->value,
                    *matrix_at(b, row, column),
                    *matrix_at(&matrices->l, row, k));
                ++iter;
            }
            *matrix_at_mut(aux_r, k, column) =
                *matrix_at(&matrices->r, k, column) - matrices->alpha * aux;
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
        if (i != 0) matrix_clear(&b); // TODO: benchmark
        matrix_b(&matrices->l, &matrices->r, &b);
        next_iter_l(matrices, &aux_l, &b);
        next_iter_r(matrices, &aux_r, &b);
        swap(&matrices->l, &aux_l);
        swap(&matrices->r, &aux_r);
    }
    matrix_free(&aux_l);
    matrix_free(&aux_r);
    return b;
}
