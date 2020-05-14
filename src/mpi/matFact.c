#include "mpi/matFact.h"

#include "common/compact_matrix.h"
#include "common/debug.h"
#include "common/matrix.h"
#include "common/parser.h"
#include "mpi/util.h"

#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <string.h>

#ifndef MPI
#    define MPI
#endif

int G_ME;

#define DELTA(a, b, lr) (2 * ((a) - (b)) * -(lr))

static void bcast_matrix(Matrix* b);

void matrix_b_full(Matrix const* l, Matrix const* r, Matrix* b) {
    assert(l->rows == r->rows);
    for (size_t i = 0; i < l->columns; i++) {
        for (size_t j = 0; j < r->columns; ++j) {
            double bij = 0;
            for (size_t k = 0; k < l->rows; ++k) {
                bij += *MATRIX_AT(l, k, i) * *MATRIX_AT(r, k, j);
            }
            *MATRIX_AT_MUT(b, i, j) = bij;
        }
    }
}

void matrix_b(
    Matrix const* const l,
    Matrix const* const r,
    Matrix* const b,
    CompactMatrix const* const a) {

    Item const* iter = a->items;
    Item const* const end = iter + a->current_items;
    while (iter != end) {
        double bij = 0;
        for (size_t k = 0; k < l->rows; k++) {
            bij += *MATRIX_AT(l, k, iter->row) * *MATRIX_AT(r, k, iter->column);
        }
        *MATRIX_AT_MUT(b, iter->row, iter->column) = bij;
        ++iter;
    }
}

void next_iter_l(
    Matrices const* const matrices,
    Matrix* const aux_l,
    Matrix const* const b,
    size_t const nK) {

    Item const* iter = matrices->a.items;
    Item const* const end = iter + matrices->a.current_items;

    while (iter != end) {
        size_t counter = 0;
        for (size_t k = 0; k < nK; ++k) {
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
        }
        iter += counter;
    }
}

void next_iter_r(
    Matrices const* const matrices,
    Matrix* const aux_r,
    Matrix const* const b,
    size_t const nK) {

    for (size_t k = 0; k < nK; ++k) {
        Item const* iter = matrices->a_transpose.items;
        Item const* const end = iter + matrices->a_transpose.current_items;
        while (iter != end) {
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

void bcast_matrix(Matrix* const b) {
    if (MPI_Bcast(
            b->data, b->columns * b->rows, MPI_DOUBLE, 0, MPI_COMM_WORLD)) {
        debug_print_backtrace("couldn't broadcast matrix");
    }
}

void send_matrix(Matrix const* const m, int const to) {
    if (MPI_Send(
            m->data, m->rows * m->columns, MPI_DOUBLE, to, 0, MPI_COMM_WORLD)) {
        debug_print_backtrace("couldn't send matrix slice");
    }
}

void receive_matrix_slice(Matrix* const m, int const from, Slice const s) {
    if (MPI_Recv(
            MATRIX_AT_MUT(m, s.start, 0),
            (s.end - s.start) * m->columns,
            MPI_DOUBLE,
            from,
            0,
            MPI_COMM_WORLD,
            NULL)) {
        debug_print_backtrace("couldn't receive matrix slice");
    }
}

// This function is only define for nprocs > l->rows
Matrix iter_mpi(
    Matrices* const matrices, int const nprocs, int const me) {
    Matrix aux_l = matrix_clone(&matrices->l);
    Matrix aux_r = matrix_clone(&matrices->r);
    Matrix b = matrix_make(matrices->a.n_rows, matrices->a.n_cols);
    for (size_t i = 0; i < matrices->num_iterations; ++i) {
        if (me == 0) {
            /* eprintln("Progress %zu/%zu", i + 1, matrices->num_iterations); */
            matrix_b(&matrices->l, &matrices->r, &b, &matrices->a);
        }
        bcast_matrix(&b);
        next_iter_l(matrices, &aux_l, &b, me == 0 ? slice_len(0, nprocs, aux_l.rows) : aux_l.rows);
        next_iter_r(matrices, &aux_r, &b, me == 0 ? slice_len(0, nprocs, aux_r.rows) : aux_r.rows);
        swap(&matrices->l, &aux_l);
        swap(&matrices->r, &aux_r);
        if (me != 0) {
            send_matrix(&matrices->l, 0);
            send_matrix(&matrices->r, 0);
        } else {
            for (int proc = 1; proc < nprocs; ++proc) {
                Slice proc_s = slice_rows(proc, nprocs, matrices->l.rows);
                receive_matrix_slice(&matrices->l, proc, proc_s);
                receive_matrix_slice(&matrices->r, proc, proc_s);
            }
        }
    }
    matrix_b_full(&matrices->l, &matrices->r, &b);
    matrix_free(&aux_l);
    matrix_free(&aux_r);
    return b;
}
