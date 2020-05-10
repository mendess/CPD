#include "mpi/matFact.h"

#include "common/compact_matrix.h"
#include "common/debug.h"
#include "common/matrix.h"
#include "common/parser.h"

#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <string.h>

#ifndef MPI
#    define MPI
#endif

int G_ME;

#define DELTA(a, b, lr) (2 * ((a) - (b)) * -(lr))

typedef struct {
    size_t start;
    size_t end;
} Slice;

static size_t start_chunk(size_t proc_id, int nprocs, size_t num_iters);
static Slice slice_rows(int proc_id, int nprocs, size_t n_rows);
static void bcast_matrix(Matrix* b);
static size_t proc_from_chunk(size_t k, int nprocs, size_t num_iters);
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
    Matrices const* const m, Matrix* b, int const me, int const nprocs) {
    assert(m->l.rows == m->r.rows);
    for (size_t i = 0; i < m->l.columns; i++) {
        for (size_t j = 0; j < m->r.columns; ++j) {
            double bij = 0;
            for (size_t k = 0; k < m->l.rows; ++k) {
                size_t const owner = proc_from_chunk(k, nprocs, m->l.rows);
                if (me == 0) {
                    if (me >= 0 && owner == (unsigned) me) {
                        bij +=
                            *MATRIX_AT(&m->l, k, i) * *MATRIX_AT(&m->r, k, j);
                    } else {
                        bij += receive_double(owner) * receive_double(owner);
                    }
                } else {
                    if (me >= 0 && owner == (unsigned) me) {
                        send_double(MATRIX_AT(&m->l, k, i), 0);
                        send_double(MATRIX_AT(&m->r, k, j), 0);
                    }
                }
            }
            *MATRIX_AT_MUT(b, i, j) = bij;
        }
    }
}

void team_matrix_b(
    Matrices const* const m, Matrix* const b, int const me, int const nprocs) {

    Item const* iter = m->a.items;
    Item const* const end = iter + m->a.current_items;
    while (iter != end) {
        double bij = 0;
        for (size_t k = 0; k < m->l.rows; k++) {
            size_t const owner = proc_from_chunk(k, nprocs, m->l.rows);
            if (me == 0) {
                if (me >= 0 && owner == (unsigned) me) {
                    bij += *MATRIX_AT(&m->l, k, iter->row) *
                           *MATRIX_AT(&m->r, k, iter->column);
                } else {
                    bij += receive_double(owner) * receive_double(owner);
                }
            } else {
                if (me >= 0 && owner == (unsigned) me) {
                    send_double(MATRIX_AT(&m->l, k, iter->row), 0);
                    send_double(MATRIX_AT(&m->r, k, iter->column), 0);
                }
            }
        }
        *MATRIX_AT_MUT(b, iter->row, iter->column) = bij;
        ++iter;
    }
}

void next_iter_l(
    Matrices const* const matrices,
    Slice const a,
    Matrix* const aux_l,
    Matrix const* const b) {

    Item const* iter = matrices->a.items;
    Item const* const end = iter + matrices->a.current_items;

    while (iter != end) {
        size_t counter = 0;
        for (size_t k = a.start; k < a.end; ++k) {
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
    Slice const a,
    Matrix* const aux_r,
    Matrix const* const b) {

    for (size_t k = a.start; k < a.end; ++k) {
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

void swap(Matrix* const a, Matrix* const b) {
    Matrix tmp = *a;
    *a = *b;
    *b = tmp;
}

size_t
start_chunk(size_t const proc_id, int const nprocs, size_t const num_iters) {
    size_t const rem = (num_iters % nprocs);
    size_t const x = proc_id * (num_iters - rem) / nprocs;
    return x + (proc_id < rem ? proc_id : rem);
}

size_t
proc_from_chunk(size_t const k, int const nprocs, size_t const num_iters) {
    for (int i = 0; i < nprocs; ++i) {
        size_t try_k = start_chunk(i, nprocs, num_iters);
        size_t try_k_end = start_chunk(i + 1, nprocs, num_iters);
        if (try_k <= k && k < try_k_end) {
            return i;
        }
    }
    eprintf("k = %zu\n", k);
    debug_print_backtrace("k out of range");
}

Slice slice_rows(int const proc_id, int const nprocs, size_t const n_rows) {
    return (Slice){
        .start = start_chunk(proc_id, nprocs, n_rows),
        .end = start_chunk(proc_id + 1, nprocs, n_rows)};
}

void bcast_matrix(Matrix* const b) {
    if (MPI_Bcast(
            b->data, b->columns * b->rows, MPI_DOUBLE, 0, MPI_COMM_WORLD)) {
        debug_print_backtrace("couldn't broadcast matrix");
    }
}

Matrix iter_mpi(Matrices* const matrices, int nprocs, int const me) {
    // This allows the program to function even if there are less rows then
    // processes
    if (nprocs >= 0 && matrices->l.rows < (unsigned) nprocs) {
        if (me != 0) {
            return (Matrix){.data = NULL, .rows = 0, .columns = 0};
        } else {
            nprocs = 1;
        }
    }
    Matrix aux_l = matrix_clone(&matrices->l);
    Matrix aux_r = matrix_clone(&matrices->r);
    Matrix b = matrix_make(matrices->a.n_rows, matrices->a.n_cols);
    Slice s = slice_rows(me, nprocs, matrices->l.rows);
    for (size_t i = 0; i < matrices->num_iterations; ++i) {
        team_matrix_b(matrices, &b, me, nprocs);
        bcast_matrix(&b);
        next_iter_l(matrices, s, &aux_l, &b);
        next_iter_r(matrices, s, &aux_r, &b);
        swap(&matrices->l, &aux_l);
        swap(&matrices->r, &aux_r);
    }
    team_matrix_b_full(matrices, &b, me, nprocs);
    matrix_free(&aux_l);
    matrix_free(&aux_r);
    return b;
}
