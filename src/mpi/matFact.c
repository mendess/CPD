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
static void send_double(double const* s, int to);
static double receive_double(int from);

static void send_double(double const* const s, int const to) {
    if (MPI_Send(s, 1, MPI_DOUBLE, to, 0, MPI_COMM_WORLD)) {
        debug_print_backtrace("send double failed");
    }
}

static double receive_double(int const from) {
    double d;
    if (MPI_Recv(&d, 1, MPI_DOUBLE, from, 0, MPI_COMM_WORLD, NULL)) {
        debug_print_backtrace("receive double failed");
    }
    return d;
}

void team_matrix_b_full(
    Matrices const* const m,
    Matrix* b,
    int const me,
    int const nprocs,
    size_t const nk) {

    assert(m->l.rows == m->r.rows);
    Slice s = slice_rows(me, nprocs, nk);
    for (size_t i = 0; i < m->l.columns; i++) {
        for (size_t j = 0; j < m->r.columns; ++j) {
            double bij = 0;
            for (size_t k = 0; k < nk; ++k) {
                if (me == 0) {
                    size_t const owner = proc_from_chunk(k, nprocs, nk);
                    if (s.start <= k && k < s.end) {
                        size_t k_ = k - s.start;
                        bij +=
                            *MATRIX_AT(&m->l, k_, i) * *MATRIX_AT(&m->r, k_, j);
                    } else {
                        bij += receive_double(owner) * receive_double(owner);
                    }
                } else {
                    if (s.start <= k && k < s.end) {
                        size_t k_ = k - s.start;
                        send_double(MATRIX_AT(&m->l, k_, i), 0);
                        send_double(MATRIX_AT(&m->r, k_, j), 0);
                    }
                }
            }
            *MATRIX_AT_MUT(b, i, j) = bij;
        }
    }
}

void team_matrix_b(
    Matrices const* const m,
    Matrix* const b,
    int const me,
    int const nprocs,
    size_t const nk) {

    Item const* iter = m->a.items;
    Item const* const end = iter + m->a.current_items;
    Slice s = slice_rows(me, nprocs, nk);
    while (iter != end) {
        double bij = 0;
        for (size_t k = 0; k < nk; k++) {
            if (me == 0) { // TODO: Lift the if up
                if (s.start <= k && k < s.end) {
                    size_t k_ = k - s.start;
                    bij += *MATRIX_AT(&m->l, k_, iter->row) *
                           *MATRIX_AT(&m->r, k_, iter->column);
                } else {
                    size_t const owner = proc_from_chunk(k, nprocs, nk);
                    bij += receive_double(owner) * receive_double(owner);
                }
            } else {
                if (s.start <= k && k < s.end) {
                    size_t k_ = k - s.start;
                    send_double(MATRIX_AT(&m->l, k_, iter->row), 0);
                    send_double(MATRIX_AT(&m->r, k_, iter->column), 0);
                }
            }
        }
        *MATRIX_AT_MUT(b, iter->row, iter->column) = bij;
        ++iter;
    }
}

void next_iter_l(
    Matrices const* const matrices,
    Matrix* const aux_l,
    Matrix const* const b) {

    Item const* iter = matrices->a.items;
    Item const* const end = iter + matrices->a.current_items;

    while (iter != end) {
        size_t counter = 0;
        for (size_t k = 0; k < matrices->l.rows; ++k) {
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
    Matrix const* const b) {

    for (size_t k = 0; k < matrices->r.rows; ++k) {
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

// This function is only define for nprocs > l->rows
Matrix iter_mpi(
    Matrices* const matrices, int const nprocs, int const me, size_t const nk) {
    Matrix aux_l = matrix_clone(&matrices->l);
    Matrix aux_r = matrix_clone(&matrices->r);
    Matrix b = matrix_make(matrices->a.n_rows, matrices->a.n_cols);
    for (size_t i = 0; i < matrices->num_iterations; ++i) {
        team_matrix_b(matrices, &b, me, nprocs, nk);
        bcast_matrix(&b);
        next_iter_l(matrices, &aux_l, &b);
        next_iter_r(matrices, &aux_r, &b);
        swap(&matrices->l, &aux_l);
        swap(&matrices->r, &aux_r);
    }
    team_matrix_b_full(matrices, &b, me, nprocs, nk);
    matrix_free(&aux_l);
    matrix_free(&aux_r);
    return b;
}
