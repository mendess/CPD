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

#define BARRIER                  \
    MPI_Barrier(MPI_COMM_WORLD); \
    MPI_Barrier(MPI_COMM_WORLD); \
    MPI_Barrier(MPI_COMM_WORLD); \
    MPI_Barrier(MPI_COMM_WORLD); \
    MPI_Barrier(MPI_COMM_WORLD);

#define ONE_BY_ONE_DO(expr)                     \
    {                                           \
        BARRIER;                                \
        for (unsigned i = 0; i < NPROCS; ++i) { \
            if (ME == i) {                      \
                (expr);                         \
            }                                   \
            BARRIER                             \
        }                                       \
    }

static MPI_Comm HORIZONTAL_COMM;
static MPI_Comm VERTICAL_COMM;
static size_t I;

void matrix_b_full(
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
                    double const* lik = VMATRIX_AT(l, i, k);
                    double const* rkj = VMATRIX_AT(r, k, j);
                    bij += *lik * *rkj;
                }
            }
            *VMATRIX_AT_MUT(b, i, j) = bij;
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
            /* double old_b = bij; */
            bij += *lik * *rkj;
            /* eprintln( */
            /*     "(%zu,%zu,%zu): %f = %f + (%f * %f)", */
            /*     iter->row, */
            /*     iter->column, */
            /*     k, */
            /*     bij, */
            /*     old_b, */
            /*     *lik, */
            /*     *rkj); */
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

    size_t last_visited_row = iter->row;
    while (iter != end) {
        uint64_t counter = 0;
        for (size_t k = 0; k < matrices->l.m.columns; ++k) {
            double delta = 0;
            Item const* line_iter = iter;
            size_t const row = line_iter->row;
            if (row - last_visited_row > 1) {
                for (last_visited_row += 1; last_visited_row < row;
                     ++last_visited_row) {
                    /* if (I == 0) */
                    /*     eprintln( */
                    /*         "Indexing (%zu, %zu) 0 values", */
                    /*         last_visited_row, */
                    /*         k); */
                    *VMATRIX_AT_MUT(aux_l, last_visited_row, k) = 0.0;
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
            /* if (I == 0) */
            /*     eprintln("Indexing (%zu, %zu) %zu values", row, k, counter);
             */
            *VMATRIX_AT_MUT(aux_l, row, k) = delta;
                /**VMATRIX_AT(&matrices->l, row, k) */
        }
        last_visited_row = iter->row;
        iter += counter;
    }
    for (last_visited_row += 1; last_visited_row < VMATRIX_ROWS(aux_l);
         ++last_visited_row) {
        for (size_t k = 0; k < aux_l->m.columns; ++k) {
            /* if (I == 0) */
            /*     eprintln("Indexing (%zu, %zu) 0 values", last_visited_row,
             * k); */
            *VMATRIX_AT_MUT(aux_l, last_visited_row, k) = 0.0;
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

void next_iter_r(
    VMatrices const* const matrices,
    VMatrix* const aux_r,
    VMatrix const* const b) {

    for (size_t k = 0; k < matrices->r.m.rows; ++k) {
        Item const* iter = matrices->a_transpose.items;
        Item const* const end = iter + matrices->a_transpose.current_items;
        size_t last_visited_column = iter->row;
        while (iter != end) {
            double delta = 0;
            size_t const column = iter->row;
            if (column - last_visited_column > 1) {
                for (last_visited_column += 1; last_visited_column < column;
                     ++last_visited_column) {
                    /* eprintln( */
                    /* "Indexing (%zu, %zu) 0 values", k, last_visited_column);
                     */
                    *VMATRIX_AT_MUT(aux_r, k, last_visited_column) = 0.0;
                }
            }
            uint64_t counter = 0;
            while (iter != end && iter->row == column) {
                size_t const row = iter->column;
                delta += DELTA(
                    iter->value,
                    *VMATRIX_AT(b, row, column),
                    *VMATRIX_AT(&matrices->l, row, k));
                ++iter;
                ++counter;
            }
            // eprintln("Indexing (%zu, %zu) %zu values", k, column, counter);
            *VMATRIX_AT_MUT(aux_r, k, column) = delta;
            last_visited_column = column;
        }
        for (last_visited_column += 1;
             last_visited_column < VMATRIX_COLS(aux_r);
             ++last_visited_column) {
            // eprintln("Indexing (%zu, %zu) 0 values", k, last_visited_column);
            *VMATRIX_AT_MUT(aux_r, k, last_visited_column) = 0.0;
        }
    }
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
    VMatrix aux_l = vmatrix_clone(&matrices->l);
    VMatrix aux_r = vmatrix_clone(&matrices->r);
    ABounds bounding_box = a_bounds(ME, matrices->a.n_rows, matrices->a.n_cols);
    VMatrix b = vmatrix_make(
        bounding_box.i.start,
        bounding_box.i.end,
        bounding_box.j.start,
        bounding_box.j.end);
    for (size_t i = 0; i < matrices->num_iterations; ++i) {
        I = i;
        /* if (ME == 0) */
        /*     eprintln("Progress %zu/%zu", i + 1, matrices->num_iterations); */
        matrix_b(&matrices->l, &matrices->r, &b, &matrices->a);
        /* if (i == 0) */
        /*     ONE_BY_ONE_DO(vmatrix_print_with_name( */
        /*         &b, "B", matrices->a.n_rows, matrices->a.n_cols)) */
        next_iter_l(matrices, &aux_l, &b);
        /* if (i == 0) { */
        /*     ONE_BY_ONE_DO(vmatrix_print_with_name( */
        /*         &matrices->l, "L", matrices->a.n_rows, 0)) */
        /* } */
        next_iter_r(matrices, &aux_r, &b);
        vswap(&matrices->l, &aux_l);
        vswap(&matrices->r, &aux_r);
    }
    vmatrix_free(&aux_l);
    vmatrix_free(&aux_r);
    /* ONE_BY_ONE_DO(vmatrix_print_with_name( */
    /*     &b, "B", matrices->a.n_rows, matrices->a.n_cols)) */
    matrix_b_full(&matrices->l, &matrices->r, &b, &matrices->a);
    if (ME == 0) {
        Matrix final_b = matrix_make(matrices->a.n_rows, matrices->a.n_cols);
        /* double const* const data = b.m.data; */
        /* double const* const end = b.m.data + (b.m.rows * b.m.columns); */
        for (unsigned proc = 0; proc < NPROCS; ++proc) {
            /* eprintln("Proc: %u", proc); */
            ABounds bounding_box =
                a_bounds(proc, matrices->a.n_rows, matrices->a.n_cols);
            vmatrix_change_offsets(
                &b,
                bounding_box.i.start,
                bounding_box.i.end,
                bounding_box.j.start,
                bounding_box.j.end);
            /* for (double const* i = data; i != end; ++i) { */
            /*     fprintf(stderr, "%.6f ", *i); */
            /* } */
            /* fputc('\n', stderr); */
            if (proc != 0) {
                size_t count = (bounding_box.i.end - bounding_box.i.start) *
                               (bounding_box.j.end - bounding_box.j.start);
                mpi_recv_doubles(b.m.data, count, proc);
            }
            /* for (double const* i = data; i != end; ++i) { */
            /*     fprintf(stderr, "%.6f ", *i); */
            /* } */
            /* fputc('\n', stderr); */
            for (size_t i = bounding_box.i.start; i < bounding_box.i.end; ++i) {
                for (size_t j = bounding_box.j.start; j < bounding_box.j.end;
                     ++j) {
                    double const* bij = VMATRIX_AT(&b, i, j);
                    /* eprintln("Indexing (%zu, %zu): %f", i, j, *bij); */
                    *MATRIX_AT_MUT(&final_b, i, j) = *bij;
                }
            }
        }
        /* matrix_print_with_name(&final_b, "After"); */
        vmatrix_free(&b);
        return final_b;
    } else {
        mpi_send_doubles(b.m.data, b.m.rows * b.m.columns, 0);
    }
    vmatrix_free(&b);
    return (Matrix){0};
}
