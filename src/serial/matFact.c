#include "matFact.h"

#include "parser.h"

#include <stdio.h>

static inline double delta(double const A, double const B, double const LR) {
    return 2 * (A - B) * (-LR);
}

void matrix_b(Matrix const* L, Matrix const* R, Matrix* matrix) {
    for (size_t i = 0; i < L->rows; i++) {
        for (size_t j = 0; j < R->columns; ++j) {
            for (size_t k = 0; k < L->columns; ++k) {
                *matrix_at_mut(matrix, i, j) +=
                    *matrix_at(L, i, k) * *matrix_at(R, k, j);
            }
        }
    }
}

void next_iterL(Matrixes const* matrixes, Matrix* aux_L, Matrix* b) {
    matrix_b(&matrixes->l, &matrixes->r, b);
    for (size_t i = 0; i < matrixes->l.rows; i++) {
        for (size_t k = 0; k < matrixes->l.columns; k++) {
            double aux = 0;
            for (size_t j = 0; j < matrixes->a.columns; j++) {
                if (*matrix_at(&matrixes->a, i, j) != 0) {
                    aux += delta(
                        *matrix_at(&matrixes->a, i, j),
                        *matrix_at(b, i, j),
                        *matrix_at(&matrixes->r, k, j));
                }
            }
            *matrix_at_mut(aux_L, i, k) =
                *matrix_at(&matrixes->l, i, k) - matrixes->alpha * aux;
        }
    }
}

void next_iterR(Matrixes const* matrixes, Matrix* aux_r, Matrix* b) {
    for (size_t k = 0; k < matrixes->r.rows; k++) {
        for (size_t j = 0; j < matrixes->r.columns; j++) {
            double aux = 0;
            for (size_t i = 0; i < matrixes->a.rows; i++) {
                if (*matrix_at(&matrixes->a, i, j) != 0) {
                    aux += delta(
                        *matrix_at(&matrixes->a, i, j),
                        *matrix_at(b, i, j),
                        *matrix_at(&matrixes->l, i, k));
                }
            }
            *matrix_at_mut(aux_r, k, j) =
                *matrix_at(&matrixes->r, k, j) - matrixes->alpha * aux;
        }
    }
}

static inline void swap(Matrix* a, Matrix* b) {
    Matrix tmp = *a;
    *b = *a;
    *a = tmp;
}

void iter(Matrixes* matrixes) {
    Matrix aux_l = matrix_make(matrixes->l.rows, matrixes->l.columns);
    Matrix aux_r = matrix_make(matrixes->r.rows, matrixes->r.columns);
    Matrix b = matrix_make(matrixes->a.rows, matrixes->a.columns);
    for (size_t i = 0; i < matrixes->num_iterations; i++) {
        next_iterL(matrixes, &aux_l, &b);
        next_iterR(matrixes, &aux_r, &b);
        swap(&matrixes->l, &aux_l);
        swap(&matrixes->r, &aux_r);
    }
    matrix_free(&aux_l);
    matrix_free(&aux_r);
}
