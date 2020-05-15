#include "mpi/matFact.h"

#include "common/compact_matrix.h"
#include "common/debug.h"
#include "common/matrix.h"
#include "common/parser.h"
#include "mpi/util.h"
#include "mpi/wrappers.h"

#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <string.h>

#ifndef MPI
#    define MPI
#endif

int G_ME;

#define DELTA(a, b, lr) (2 * ((a) - (b)) * -(lr))

void matrix_b_full(VMatrix const* l, VMatrix const* r, Matrix* b) {
    assert(l->m.columns == r->m.rows);
    for (size_t i = 0; i < l->m.rows; i++) {
        for (size_t j = 0; j < r->m.columns; ++j) {
            double bij = 0;
            for (size_t k = 0; k < l->m.columns; ++k) {
                double const* lik = VMATRIX_AT(l, i, k);
                double const* rkj = VMATRIX_AT(r, k, j);
                bij += *lik * *rkj;
            }
            *MATRIX_AT_MUT(b, i, j) = bij;
        }
    }
}

void matrix_b(
    VMatrix const* const l,
    VMatrix const* const r,
    VMatrix* const b,
    CompactMatrix const* const a) {

    Item const* iter = a->items;
    Item const* const end = iter + a->current_items;
    while (iter != end) {
        double bij = 0;
        for (size_t k = 0; k < l->m.columns; ++k) {
            double const* lik = VMATRIX_AT(l, iter->row, k);
            double const* rkj = VMATRIX_AT(r, k, iter->column);
            bij += *lik * *rkj;
        }
        *VMATRIX_AT_MUT(b, iter->row, iter->column) = bij;
        ++iter;
    }
}

void next_iter_l(
    VMatrices const* const matrices,
    VMatrix* const aux_l,
    VMatrix const* const b) {

    Item const* iter = matrices->a.items;
    Item const* const end = iter + matrices->a.current_items;

    while (iter != end) {
        size_t counter = 0;
        for (size_t k = 0; k < matrices->l.m.columns; ++k) {
            double aux = 0;
            Item const* line_iter = iter;
            size_t const row = line_iter->row;
            counter = 0;
            while (line_iter != end && line_iter->row == row) {
                size_t const column = line_iter->column;
                aux += DELTA(
                    line_iter->value,
                    *VMATRIX_AT(b, row, column),
                    *VMATRIX_AT(&matrices->r, k, column));
                ++line_iter;
                ++counter;
            }
            *VMATRIX_AT_MUT(aux_l, row, k) =
                *VMATRIX_AT(&matrices->l, row, k) - matrices->alpha * aux;
        }
        iter += counter;
    }
}

void next_iter_r(
    VMatrices const* const matrices,
    VMatrix* const aux_r,
    VMatrix const* const b) {

    for (size_t k = 0; k < matrices->r.m.rows; ++k) {
        Item const* iter = matrices->a_transpose.items;
        Item const* const end = iter + matrices->a_transpose.current_items;
        while (iter != end) {
            double aux = 0;
            size_t const column = iter->row;
            while (iter != end && iter->row == column) {
                size_t const row = iter->column;
                aux += DELTA(
                    iter->value,
                    *VMATRIX_AT(b, row, column),
                    *VMATRIX_AT(&matrices->l, row, k));
                ++iter;
            }
            *VMATRIX_AT_MUT(aux_r, k, column) =
                *VMATRIX_AT(&matrices->r, k, column) - matrices->alpha * aux;
        }
    }
}

// This function is only defined for NPROCS > l->rows
Matrix iter_mpi(VMatrices* const matrices) {
    VMatrix aux_l = vmatrix_clone(&matrices->l);
    VMatrix aux_r = vmatrix_clone(&matrices->r);
    ABounds bounding_box = a_bounds(ME, matrices->a.n_rows, matrices->a.n_cols);
    VMatrix b = vmatrix_make(
        bounding_box.i.start,
        bounding_box.i.end,
        bounding_box.j.start,
        bounding_box.j.end);
    for (size_t i = 0; i < matrices->num_iterations; ++i) {
        /* eprintln("Progress %zu/%zu", i + 1, matrices->num_iterations); */
        matrix_b(&matrices->l, &matrices->r, &b, &matrices->a);
        next_iter_l(matrices, &aux_l, &b);
        next_iter_r(matrices, &aux_r, &b);
        vswap(&matrices->l, &aux_l);
        vswap(&matrices->r, &aux_r);
    }
    vmatrix_free(&aux_l);
    vmatrix_free(&aux_r);
    if (ME == 0) {
        Matrix final_b = matrix_make(matrices->a.n_rows, matrices->a.n_cols);
        for (unsigned proc = 0; proc < NPROCS; ++proc) {
            ABounds bounding_box =
                a_bounds(proc, matrices->a.n_rows, matrices->a.n_cols);
            vmatrix_change_offsets(
                &b,
                bounding_box.i.start,
                bounding_box.i.end,
                bounding_box.j.start,
                bounding_box.j.end);
            if (proc != 0) {
                size_t count = (bounding_box.i.end - bounding_box.i.start) *
                               (bounding_box.j.end - bounding_box.j.start);
                mpi_recv_doubles(b.m.data, count, proc);
            }
            for (size_t i = bounding_box.i.start; i < bounding_box.i.end; ++i) {
                for (size_t j = bounding_box.j.start; j < bounding_box.j.end;
                     ++j) {
                    double const* bij = VMATRIX_AT(&b, i, j);
                    *MATRIX_AT_MUT(&final_b, i, j) = *bij;
                }
            }
        }
        matrix_b_full(&matrices->l, &matrices->r, &final_b);
        vmatrix_free(&b);
        return final_b;
    } else {
        mpi_send_doubles(b.m.data, b.m.rows * b.m.columns, 0);
    }
    vmatrix_free(&b);
    return (Matrix){0};
}
