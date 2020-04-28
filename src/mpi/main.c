#include <stdio.h>
#include <stdlib.h>
#include "matFact.h"
#include "parser.h"
#include <mpi.h>

int main(int argc, char const** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
        return EXIT_FAILURE;
    }
    int me, nprocs;
    MPI_Init(&argc, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    Matrices matrices;
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

    if (me == 0){
        int lines_per_node_prime = matrices.a_prime.n_rows / nprocs;
        int lines_per_node_transpose = matrices.a_prime_transpose.n_rows / nprocs;
        int prime_rest = matrices.a_prime.n_rows % nprocs;
        int transpose_rest = matrices.a_prime_transpose.n_rows % nprocs;
        printf("Number of lines %d\n", matrices.a_prime.n_rows);
        printf("Lines per node %d\n", lines_per_node_prime);
        printf("Lines for node 0: start-%d end-%d\n", lines_per_node_prime * (nprocs - 1), matrices.a_prime.n_rows - 1);
        for(int i = 1; i < nprocs; i++){
            int start = lines_per_node_prime * (i - 1);
            int end = lines_per_node_prime*i - 1;
            printf("Lines for node %d: start-%d end-%d\n", i, start, end);
            int n_elems = matrices.a_prime.row_pos[end + 1] - 1 - matrices.a_prime.row_pos[start];
            MPI_Send(&n_elems, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            //printf("Node %d will receive %d elements\n", i, matrices.a_prime.row_pos[end + 1] - 1 - matrices.a_prime.row_pos[start]);
        }
    } else {
        int n_elems;
        MPI_Recv(&n_elems, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);
        printf("Node %d receives %d elems\n", me, n_elems);
    }
    /*
    for(int i = 0; i < matrices.a_prime.n_rows; i++){
        printf("Row %d starts at position %ld\n", i, matrices.a_prime.row_pos[i]);
    }
    */
    MPI_Finalize();
}
