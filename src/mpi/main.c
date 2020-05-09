#include "common/debug.h"
#include "common/parser.h"
#include "mpi/cmatrix.h"
#include "mpi/matFact.h"
#include "mpi/mpi_size_t.h"
#include "mpi/parser.h"

#include <assert.h>
#include <limits.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NSIZE_T 11

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
        return EXIT_FAILURE;
    }
    int me, nprocs;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    Matrices matrices = {0};
    G_ME = me;
    if (me == 0) eprintf("Filename: %s\n", argv[1]);
    eprintf("PID: %d\n", getpid());

    if (me == 0) {
        ParserError error = parse_file_lt(argv[1], &matrices);
        switch (error) {
            case PARSER_ERROR_IO:
                fputs("IO Error\n", stderr);
                return EXIT_FAILURE;
            case PARSER_ERROR_INVALID_FORMAT:
                fputs("Format Error\n", stderr);
                return EXIT_FAILURE;
            default:
                break;
        }
        size_t send_size_t[NSIZE_T] = {
            matrices.num_iterations,
            matrices.a.current_items,
            matrices.a.n_rows,
            matrices.a.n_cols,
            matrices.a_transpose.current_items,
            matrices.a_transpose.n_rows,
            matrices.a_transpose.n_cols,
            matrices.l.rows,
            matrices.l.columns,
            matrices.r.rows,
            matrices.r.columns};
        for (int i = 1; i < nprocs; i++) {
            MPI_Send(send_size_t, NSIZE_T, MPI_SIZE_T, i, 0, MPI_COMM_WORLD);
            mpi_send_items(
                matrices.a.items,
                matrices.a.current_items,
                i,
                0,
                MPI_COMM_WORLD);
            mpi_send_items(
                matrices.a_transpose.items,
                matrices.a_transpose.current_items,
                i,
                0,
                MPI_COMM_WORLD);
            MPI_Send(
                matrices.l.data,
                matrices.l.rows * matrices.l.columns,
                MPI_DOUBLE,
                i,
                0,
                MPI_COMM_WORLD);
            MPI_Send(
                matrices.r.data,
                matrices.r.rows * matrices.r.columns,
                MPI_DOUBLE,
                i,
                0,
                MPI_COMM_WORLD);
            MPI_Send(&matrices.alpha, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
        }
    } else {
        size_t send_size_t[NSIZE_T];
        MPI_Recv(send_size_t, NSIZE_T, MPI_SIZE_T, 0, 0, MPI_COMM_WORLD, NULL);
        matrices.num_iterations = send_size_t[0];
        matrices.a.current_items = send_size_t[1];
        matrices.a.n_rows = send_size_t[2];
        matrices.a.n_cols = send_size_t[3];
        matrices.a_transpose.current_items = send_size_t[4];
        matrices.a_transpose.n_rows = send_size_t[5];
        matrices.a_transpose.n_cols = send_size_t[6];

        matrices.a.items = malloc(sizeof(Item) * matrices.a.current_items);
        matrices.a_transpose.items =
            malloc(sizeof(Item) * matrices.a_transpose.current_items);
        matrices.l = matrix_make(send_size_t[7], send_size_t[8]);
        matrices.r = matrix_make(send_size_t[9], send_size_t[10]);

        mpi_recv_items(
            matrices.a.items,
            matrices.a.current_items,
            0,
            0,
            MPI_COMM_WORLD,
            NULL);
        mpi_recv_items(
            matrices.a_transpose.items,
            matrices.a_transpose.current_items,
            0,
            0,
            MPI_COMM_WORLD,
            NULL);
        MPI_Recv(
            matrices.l.data,
            matrices.l.rows * matrices.l.columns,
            MPI_DOUBLE,
            0,
            0,
            MPI_COMM_WORLD,
            NULL);
        MPI_Recv(
            matrices.r.data,
            matrices.r.rows * matrices.r.columns,
            MPI_DOUBLE,
            0,
            0,
            MPI_COMM_WORLD,
            NULL);
        MPI_Recv(&matrices.alpha, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, NULL);
    }
    random_fill_LT_R(&matrices.l, &matrices.r);
    Matrix b = iter_mpi(&matrices, nprocs, me);
    if (me == 0) print_output(&matrices, &b);
    matrix_free(&b);
    matrices_free(&matrices);

    MPI_Finalize();
    return 0;
}
