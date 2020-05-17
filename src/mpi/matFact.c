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

#define DELTA(a, b, lr) (2 * ((a) - (b)) * -(lr))

static MPI_Comm HORIZONTAL_COMM;
static MPI_Comm VERTICAL_COMM;

static void matrix_b_full(
    VMatrix const* const l,
    VMatrix const* const r,
    VMatrix* const b,
    CompactMatrix const* const a) {

    assert(l->m.columns == r->m.rows);
    ABounds bounding_box = a_bounds(ME, a->n_rows, a->n_cols);
    Item const* iter = a->items;
    Item const* const end = iter + a->current_items;
    for (size_t i = bounding_box.i.start; i < bounding_box.i.end; i++) {
        for (size_t j = bounding_box.j.start; j < bounding_box.j.end; ++j) {
            double bij = 0;
            if (iter != end && iter->row == i && iter->column == j) {
                ++iter;
            } else {
                for (size_t k = 0; k < l->m.columns; ++k) {
                    bij += *VMATRIX_AT(l, i, k) * *VMATRIX_AT(r, k, j);
                }
            }
            *VMATRIX_AT_MUT(b, i, j) = bij;
        }
    }
}

static void matrix_b(
    VMatrix const* const l,
    VMatrix const* const r,
    VMatrix* const b,
    CompactMatrix const* const a) {

    Item const* iter = a->items;
    Item const* const end = iter + a->current_items;
    while (iter != end) {
        double bij = 0;
        for (size_t k = 0; k < l->m.columns; ++k) {
            bij +=
                *VMATRIX_AT(l, iter->row, k) * *VMATRIX_AT(r, k, iter->column);
        }
        *VMATRIX_AT_MUT(b, iter->row, iter->column) = bij;
        ++iter;
    }
}

static void next_iter_l(
    VMatrices const* const matrices,
    VMatrix* const aux_l,
    VMatrix const* const b,
    ABounds const* const bounds) {

    Item const* iter = matrices->a.items;
    Item const* const end = iter + matrices->a.current_items;

    size_t row_to_visit = bounds->i.start;
    while (iter != end) {
        uint64_t counter = 0;
        size_t const row = iter->row;
        for (size_t k = 0; k < matrices->l.m.columns; ++k) {
            double delta = 0;
            Item const* line_iter = iter;
            if (row_to_visit != row) {
                for (; row_to_visit < row; ++row_to_visit) {
                    for (size_t k = 0; k < aux_l->m.columns; ++k) {
                        *VMATRIX_AT_MUT(aux_l, row_to_visit, k) = 0.0;
                    }
                }
            }
            counter = 0;
            while (line_iter != end && line_iter->row == row) {
                size_t const column = line_iter->column;
                delta += DELTA(
                    line_iter->value,
                    *VMATRIX_AT(b, row, column),
                    *VMATRIX_AT(&matrices->r, k, column));
                ++line_iter;
                ++counter;
            }
            *VMATRIX_AT_MUT(aux_l, row, k) = delta;
        }
        ++row_to_visit;
        iter += counter;
    }
    size_t aux_rows = VMATRIX_ROWS(aux_l);
    // TODO: memset
    for (; row_to_visit < aux_rows; ++row_to_visit) {
        for (size_t k = 0; k < aux_l->m.columns; ++k) {
            *VMATRIX_AT_MUT(aux_l, row_to_visit, k) = 0.0;
        }
    }

    MPI_Allreduce(
        MPI_IN_PLACE,
        aux_l->m.data,
        aux_l->m.rows * aux_l->m.columns,
        MPI_DOUBLE,
        MPI_SUM,
        HORIZONTAL_COMM);

    double const* const aux_l_end =
        aux_l->m.data + aux_l->m.rows * aux_l->m.columns;
    double const* l_iter = matrices->l.m.data;

    for (double* i = aux_l->m.data; i != aux_l_end; ++i, ++l_iter) {
        *i = *l_iter - matrices->alpha * *i;
    }
}

static void next_iter_r(
    VMatrices const* const matrices,
    VMatrix* const aux_r,
    VMatrix const* const b,
    ABounds const* const bounds) {

    size_t aux_cols = VMATRIX_COLS(aux_r);
    // calcular os deltas
    for (size_t k = 0; k < matrices->r.m.rows; ++k) {
        Item const* iter = matrices->a_transpose.items;
        Item const* const end = iter + matrices->a_transpose.current_items;
        size_t column_to_visit = bounds->j.start;
        // For each non-zero element of A (for j in A.cols)
        while (iter != end) {
            double delta = 0.0;
            size_t const column = iter->row;
            // compensar colunas que possao nao existir na matrix A que tenho
            if (column_to_visit != column) {
                for (; column_to_visit < column; ++column_to_visit) {
                    *VMATRIX_AT_MUT(aux_r, k, column_to_visit) = 0.0;
                }
            }
            // For each line of A (for i in a.rows)
            while (iter != end && iter->row == column) {
                size_t const row = iter->column;
                delta += DELTA(
                    iter->value,
                    *VMATRIX_AT(b, row, column),
                    *VMATRIX_AT(&matrices->l, row, k));
                ++iter;
            }
            *VMATRIX_AT_MUT(aux_r, k, column) = delta;
            ++column_to_visit;
        }
        // compensar colunas que possao nao existir na matrix A que tenho
        for (; column_to_visit < aux_cols; ++column_to_visit) {
            *VMATRIX_AT_MUT(aux_r, k, column_to_visit) = 0.0;
        }
    }

    // somar os deltas dos outros processos
    MPI_Allreduce(
        MPI_IN_PLACE,
        aux_r->m.data,
        aux_r->m.rows * aux_r->m.columns,
        MPI_DOUBLE,
        MPI_SUM,
        VERTICAL_COMM);

    double const* const aux_r_end =
        aux_r->m.data + aux_r->m.rows * aux_r->m.columns;
    double const* r_iter = matrices->r.m.data;
    // R (t+1) = R (t) - alpha * delta
    for (double* delta = aux_r->m.data; delta != aux_r_end; ++delta, ++r_iter) {
        *delta = *r_iter - matrices->alpha * *delta;
    }
}

// This function is only defined for NPROCS > l->rows
Matrix iter_mpi(VMatrices* const matrices) {
    unsigned hor_color = ME / CHECKER_BOARD_SIDE;
    unsigned ver_color = CHECKER_BOARD_SIDE + (ME % CHECKER_BOARD_SIDE);
    MPI_Comm_split(MPI_COMM_WORLD, hor_color, ME, &HORIZONTAL_COMM);
    MPI_Comm_split(MPI_COMM_WORLD, ver_color, ME, &VERTICAL_COMM);
    VMatrix aux_l = vmatrix_shallow_clone(&matrices->l);
    VMatrix aux_r = vmatrix_shallow_clone(&matrices->r);
    ABounds bounding_box = a_bounds(ME, matrices->a.n_rows, matrices->a.n_cols);
    VMatrix b = vmatrix_make(
        bounding_box.i.start,
        bounding_box.i.end,
        bounding_box.j.start,
        bounding_box.j.end);
    for (size_t i = 0; i < matrices->num_iterations; ++i) {
        matrix_b(&matrices->l, &matrices->r, &b, &matrices->a);
        next_iter_l(matrices, &aux_l, &b, &bounding_box);
        next_iter_r(matrices, &aux_r, &b, &bounding_box);
        vswap(&matrices->l, &aux_l);
        vswap(&matrices->r, &aux_r);
    }
    vmatrix_free(&aux_l);
    vmatrix_free(&aux_r);
    matrix_b_full(&matrices->l, &matrices->r, &b, &matrices->a);
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
        vmatrix_free(&b);
        return final_b;
    } else {
        mpi_send_doubles(b.m.data, b.m.rows * b.m.columns, 0);
    }
    vmatrix_free(&b);
    return (Matrix){0};
}
