#include "common/debug.h"
#include "common/parser.h"
#include "mpi/cmatrix.h"
#include "mpi/matFact.h"
#include "mpi/parser.h"
#include "mpi/util.h"
#include "mpi/wrappers.h"
#include "serial/matFact.h"

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
    MPI_Init(&argc, &argv);
    int me, nprocs;
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    assert(me >= 0);
    assert(nprocs >= 0);
    ME = (unsigned) me;
    NPROCS = (unsigned) nprocs;
    NPROCS = floor(sqrt(NPROCS));
    NPROCS *= NPROCS;
    CHECKER_BOARD_SIDE = sqrt(NPROCS);
    VMatrices matrices = {0};
    G_ME = ME;

    MPI_Group world_group;
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
    MPI_Group new_group;
    int ranges[3] = {NPROCS, nprocs - 1, 1};
    MPI_Group_range_excl(world_group, 1, &ranges, &new_group);
    MPI_Comm new_world;
    MPI_Comm_create(MPI_COMM_WORLD, new_group, &new_world);

    if (new_world == MPI_COMM_NULL)
        goto FINALIZE;
    else
        WORLD_COMM = new_world;

    if (ME == 0) {
        eprintln("Filename:           %s", argv[1]);
        eprintln("# Processes:        %u", NPROCS);
        eprintln("CheckerBoard side:  %u", CHECKER_BOARD_SIDE);
    }
    eprintln("PID: %d", getpid());

    if (ME == 0) {
        ParserError error = parse_file_rt(argv[1], &matrices);
        switch (error) {
            case PARSER_ERROR_IO:
                eputln("IO Error");
                return EXIT_FAILURE;
            case PARSER_ERROR_INVALID_FORMAT:
                eputln("Format Error");
                return EXIT_FAILURE;
            case PARSER_ERROR_OK:
                break;
            default:
                break;
        }
    } else {
        recv_parsed_file(&matrices);
    }
    if (ME == 0) {
        eprintln("A Rows:             %zu", matrices.a.n_rows);
        eprintln("A Columns:          %zu", matrices.a.n_cols);
        eprintln(
            "Distributed:        %s",
            should_work_alone(matrices.a.n_rows, matrices.a.n_cols)
                ? "\x1b[31mno\x1b[0m"
                : "\x1b[32myes\x1b[0m");
    }
    if (should_work_alone(matrices.a.n_rows, matrices.a.n_cols)) {
        if (ME == 0) {
            random_fill_LR(&matrices.l.m, &matrices.r.m);
            Matrices normal_matrices = matrices_from_vmatrices(matrices);
            NPROCS = 1;
            Matrix b = iter(&normal_matrices);
            print_output(&matrices.a, &b);
            matrix_free(&b);
        }
    } else {
        random_fill_LR_parts(&matrices.l.m, &matrices.r.m, &matrices.a);
        Matrix b = iter_mpi(&matrices);
        if (ME == 0)
            print_output(&matrices.a, &b);
        matrix_free(&b);
    }

    vmatrices_free(&matrices);

FINALIZE:
    eputln("Finalizing");
    MPI_Finalize();
    return 0;
}
