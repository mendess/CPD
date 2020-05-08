#include "mpi/matFact.h"

#include "common/compact_matrix.h"
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
    Matrix const* l, Matrix const* r, Matrix* b, CompactMatrix const* a) {
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
    Matrices const* matrices, Slice a, Matrix* aux_l, Matrix const* b) {
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
    Matrices const* matrices, Slice a, Matrix* aux_r, Matrix const* b) {
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

static inline Slice slice_rows(int proc_id, int nprocs, size_t const n_rows) {
    return (Slice){
        .start = start_chunk(proc_id, nprocs, n_rows),
        .end = start_chunk(proc_id + 1, nprocs, n_rows)};
}

static inline int send_matrix(Matrix const* b, int to) {
    return MPI_Send(
        b->data, b->columns * b->rows, MPI_DOUBLE, to, 0, MPI_COMM_WORLD);
}

static inline int send_matrix_slice(Matrix const* m, int to, Slice s) {
    return MPI_Send(
        MATRIX_AT(m, s.start, 0),
        (s.end - s.start) * m->columns,
        MPI_DOUBLE,
        to,
        0,
        MPI_COMM_WORLD);
}

static inline int receive_matrix(Matrix* b, int from) {
    return MPI_Recv(
        b->data,
        b->columns * b->rows,
        MPI_DOUBLE,
        from,
        0,
        MPI_COMM_WORLD,
        NULL);
}

static inline int receive_matrix_slice(Matrix* m, int from, Slice s) {
    return MPI_Recv(
        MATRIX_AT_MUT(m, s.start, 0),
        (s.end - s.start) * m->columns,
        MPI_DOUBLE,
        from,
        0,
        MPI_COMM_WORLD,
        NULL);
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
        if (me == 0) {
            matrix_b(&matrices->l, &matrices->r, &b, &matrices->a);
            for (int proc = 1; proc < nprocs; ++proc) send_matrix(&b, proc);
        } else {
            receive_matrix(&b, 0);
        }
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
                    &matrices->l, proc, slice_rows(proc, nprocs, matrices->l.rows));
            }
            for (int proc = 1; proc < nprocs; ++proc) {
                receive_matrix_slice(
                    &matrices->r, proc, slice_rows(proc, nprocs, matrices->l.rows));
            }
        }
    }
    matrix_b_full(&matrices->l, &matrices->r, &b);
    matrix_free(&aux_l);
    matrix_free(&aux_r);
    return b;
}
