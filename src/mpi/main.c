#include "cmatrix.h"
#include "debug.h"
#include "matFact.h"
#include "mpi_size_t.h"
#include "parser.h"

#include <assert.h>
#include <limits.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

    if (me == 0) {
        ParserError error = parse_file(argv[1], &matrices);
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

        // Send num of iterations
        // printf("Num of iterations node 0: %zu\n", matrices.num_iterations);
        // printf("Node 0 - l rows %zu \n", matrices.l.rows);

        size_t send_size_t[NSIZE_T] = {
            matrices.num_iterations,
            matrices.a._total_items,
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
        matrices.l.rows = send_size_t[7];
        matrices.l.columns = send_size_t[8];
        matrices.r.rows = send_size_t[9];
        matrices.r.columns = send_size_t[10];
        // printf("Num of iterations %zu\n", matrices.num_iterations);
        // printf("Matrix l num of rows %zu \n", matrices.l.rows);

        matrices.a.items = malloc(sizeof(Item) * matrices.a.current_items);
        matrices.a_transpose.items =
            malloc(sizeof(Item) * matrices.a_transpose.current_items);

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

        matrices.l.data =
            malloc(sizeof(double) * (matrices.l.rows * matrices.l.columns));
        matrices.r.data =
            malloc(sizeof(double) * (matrices.r.rows * matrices.r.columns));

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
    /* CompactMatrix* mm[2] = {&matrices.a, &matrices.a_transpose};
     */
    /* char c[2] = {'a', 't'}; */
    /* for (size_t m = 0; m < 2; ++m) { */
    /*     Item const* const end = m[mm]->items + m[mm]->current_items; */
    /*     for (Item const* iter = m[mm]->items; iter != end; iter++) { */
    /*         fprintf( */
    /*             stderr, */
    /*             "Line from %c node %d, pos (%zu, %zu) has value %f\n", */
    /*             c[m], */
    /*             me, */
    /*             iter->row, */
    /*             iter->column, */
    /*             iter->value); */
    /*     } */
    /* } */
    /*
    for(int i = 0; i < matrices.a.n_rows; i++){
        printf("Row %d starts at position %ld\n", i,
    matrices.a.row_pos[i]);
    }
    */
    random_fill_LR(&matrices.l, &matrices.r);
    if (me == 0) {
        Matrix b = iter_mpi(&matrices, nprocs, me);
        print_output(&matrices, &b);
        matrix_free(&b);
    }
    matrices_free(&matrices);

    MPI_Finalize();
    return 0;
}
