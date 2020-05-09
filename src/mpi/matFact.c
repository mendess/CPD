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
static MPI_Request* bcast_matrix(Matrix* b, MPI_Request* request);
static void send_matrix_slice(Matrix const* m, int to, Slice s);
static void receive_matrix_slice(Matrix* m, int from, Slice s);

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

/* void team_matrix_b( */
/*     Matrix const* const l, */
/*     Matrix const* const r, */
/*     Matrix* const b, */
/*     CompactMatrix const* const a, */
/*     int const me, */
/*     int const nprocs) { */

/*     Item const* iter = a->items; */
/*     Item const* const end = iter + a->current_items; */
/*     int teammate = 0; */
/*     Slice s = slice_rows(me, nprocs, l->rows); */
/*     while (teammate < me) { */
/*     } */
/*     while (teammate == me) { */
/*     } */
/*     while (iter != end) { */
/*         if (teammate < me) { */
/*         } else if (teammate == me) { */
/*             // I got this */
/*         } else { */
/*             // get */
/*         } */
/*         /1* double bij = 0; *1/ */
/*         for (size_t k = 0; k < l->rows; k++) { */
/*             eprintf("(i,j,k) = (%zu, %zu, %zu)\n", iter->row, iter->column,
 * k); */
/*             /1* bij += *MATRIX_AT(l, k, iter->row) * *MATRIX_AT(r, k, */
/*              * iter->column); *1/ */
/*         } */
/*         /1* *MATRIX_AT_MUT(b, iter->row, iter->column) = bij; *1/ */
/*         ++iter; */
/*     } */
/* } */

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

Slice slice_rows(int const proc_id, int const nprocs, size_t const n_rows) {
    return (Slice){
        .start = start_chunk(proc_id, nprocs, n_rows),
        .end = start_chunk(proc_id + 1, nprocs, n_rows)};
}

MPI_Request* bcast_matrix(Matrix* const b, MPI_Request* const request) {
    if (MPI_Ibcast(
            b->data,
            b->columns * b->rows,
            MPI_DOUBLE,
            0,
            MPI_COMM_WORLD,
            request)) {
        debug_print_backtrace("couldn't broadcast matrix");
    }
    return request;
}

void send_matrix_slice(Matrix const* const m, int const to, Slice const s) {
    if (MPI_Send(
            MATRIX_AT(m, s.start, 0),
            (s.end - s.start) * m->columns,
            MPI_DOUBLE,
            to,
            0,
            MPI_COMM_WORLD)) {
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
        MPI_Request b_broadcast;
        /* team_matrix_b(&matrices->l, &matrices->r, &b, &matrices->a); */
        if (me == 0) {
            matrix_b(&matrices->l, &matrices->r, &b, &matrices->a);
        }
        bcast_matrix(&b, &b_broadcast);
        next_iter_l(matrices, s, &aux_l, &b);
        next_iter_r(matrices, s, &aux_r, &b);
        swap(&matrices->l, &aux_l);
        swap(&matrices->r, &aux_r);
        if (me != 0) {
            send_matrix_slice(&matrices->l, 0, s);
            send_matrix_slice(&matrices->r, 0, s);
        } else {
            for (int proc = 1; proc < nprocs; ++proc) {
                receive_matrix_slice(
                    &matrices->l,
                    proc,
                    slice_rows(proc, nprocs, matrices->l.rows));
            }
            for (int proc = 1; proc < nprocs; ++proc) {
                receive_matrix_slice(
                    &matrices->r,
                    proc,
                    slice_rows(proc, nprocs, matrices->l.rows));
            }
        }
        MPI_Wait(&b_broadcast, NULL);
    }
    matrix_b_full(&matrices->l, &matrices->r, &b);
    matrix_free(&aux_l);
    matrix_free(&aux_r);
    return b;
}
