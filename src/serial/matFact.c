#include "matFact.h"

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
    for (size_t i = 0; i < matrices->l.rows; i++) {
        for (size_t k = 0; k < matrices->l.columns; k++) {
            double aux = 0;
            for (size_t j = 0; j < matrices->a.columns; j++) {
                if (*matrix_at(&matrices->a, i, j) != 0) {
                    aux += delta(
                        *matrix_at(&matrices->a, i, j),
                        *matrix_at(b, i, j),
                        *matrix_at(&matrices->r, k, j));
                }
            }
            *matrix_at_mut(aux_l, i, k) =
                *matrix_at(&matrices->l, i, k) - matrices->alpha * aux;
        }
    }
}

void next_iter_r(Matrices const* matrices, Matrix* aux_r, Matrix const* b) {
    for (size_t k = 0; k < matrices->r.rows; k++) {
        for (size_t j = 0; j < matrices->r.columns; j++) {
            double aux = 0;
            for (size_t i = 0; i < matrices->a.rows; i++) {
                if (*matrix_at(&matrices->a, i, j) != 0) {
                    aux += delta(
                        *matrix_at(&matrices->a, i, j),
                        *matrix_at(b, i, j),
                        *matrix_at(&matrices->l, i, k));
                }
            }
            *matrix_at_mut(aux_r, k, j) =
                *matrix_at(&matrices->r, k, j) - matrices->alpha * aux;
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
    Matrix b = matrix_make(matrices->a.rows, matrices->a.columns);
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
