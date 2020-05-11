#include "common/debug.h"
#include "common/parser.h"
#include "mpi/cmatrix.h"
#include "mpi/matFact.h"
#include "mpi/mpi_size_t.h"
#include "mpi/parser.h"
#include "mpi/util.h"

#include <assert.h>
#include <limits.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct SizeTPacket {
    size_t num_iterations;
    size_t num_items;
    size_t a_nrows;
    size_t a_ncols;
    size_t k;
    size_t i;
    size_t j;
} SizeTPacket;
#define NSizeT (sizeof(SizeTPacket) / sizeof(size_t))

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
    if (me == 0)
        eprintln("Filename: %s", argv[1]);
    eprintln("PID: %d", getpid());

    if (me == 0) {
        ParserError error = parse_file_lt(argv[1], &matrices);
        switch (error) {
            case PARSER_ERROR_IO:
                eputln("IO Error");
                return EXIT_FAILURE;
            case PARSER_ERROR_INVALID_FORMAT:
                eputln("Format Error");
                return EXIT_FAILURE;
            default:
                break;
        }
    }
    SizeTPacket packet = {
        .num_iterations = matrices.num_iterations,
        .num_items = matrices.a.current_items,
        .a_nrows = matrices.a.n_rows,
        .a_ncols = matrices.a.n_cols,
        .k = matrices.l.rows,
        .i = matrices.l.columns,
        .j = matrices.r.columns};
    MPI_Bcast(&packet, NSizeT, MPI_SIZE_T, 0, MPI_COMM_WORLD);
    if (nprocs >= 0 && (unsigned) nprocs > packet.k) {
        if (me == 0) {
            matrices.l = matrix_make(packet.k, packet.i);
            matrices.r = matrix_make(packet.k, packet.j);
            random_fill_LT_R(&matrices.l, &matrices.r);
            Matrix b = iter_mpi(&matrices, 1, me, packet.k);
            print_output(&matrices, &b);
            matrix_free(&b);
        }
    } else {
        if (me != 0) {
            matrices.a.items = malloc(sizeof(Item) * packet.num_items);
            matrices.a_transpose.items =
                malloc(sizeof(Item) * packet.num_items);
        }
        matrices.num_iterations = packet.num_iterations;
        matrices.a.current_items = matrices.a_transpose.current_items =
            packet.num_items;
        matrices.a.n_rows = matrices.a_transpose.n_cols = packet.a_nrows;
        matrices.a.n_cols = matrices.a_transpose.n_rows = packet.a_ncols;

        cmatrix_bcast_items(matrices.a.items, packet.num_items, 0);
        cmatrix_bcast_items(matrices.a_transpose.items, packet.num_items, 0);
        MPI_Bcast(&matrices.alpha, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        size_t k = slice_len(me, nprocs, packet.k);
        matrices.l = matrix_make(k, packet.i);
        matrices.r = matrix_make(k, packet.j);

        random_fill_LT_R_mpi(&matrices.l, &matrices.r, me, nprocs, packet.k);
        Matrix b = iter_mpi(&matrices, nprocs, me, packet.k);
        if (me == 0)
            print_output(&matrices, &b);
        matrix_free(&b);
    }

    matrices_free(&matrices);

    MPI_Finalize();
    return 0;
}
