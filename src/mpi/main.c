#include "cmatrix.h"
#include "mpi_size_t.h"
#include "parser.h"

#include <assert.h>
#include <mpi.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#if SIZE_MAX == UCHAR_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_CHAR
#elif SIZE_MAX == USHRT_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_SHORT
#elif SIZE_MAX == UINT_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED
#elif SIZE_MAX == ULONG_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_LONG
#elif SIZE_MAX == ULLONG_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_LONG_LONG
#else
   #error "what is happening here?"
#endif
int main(int argc, char ** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
        return EXIT_FAILURE;
    }
    int me, nprocs;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    Matrices matrices;

    if (me == 0){
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

        //Send num of iterations
        //printf("Num of iterations node 0: %zu\n", matrices.num_iterations);
        //printf("Node 0 - l rows %zu \n", matrices.l.rows);

        size_t send_size_t[11] = {matrices.num_iterations, matrices.a_prime.total_items, matrices.a_prime.n_rows, matrices.a_prime.n_cols,
        matrices.a_prime_transpose.total_items, matrices.a_prime_transpose.n_rows, matrices.a_prime_transpose.n_cols,
        matrices.l.rows, matrices.l.columns,
        matrices.r.rows, matrices.r.columns};
        for(int i = 1; i < nprocs; i++){
            MPI_Send(send_size_t, 11, my_MPI_SIZE_T, i, 0, MPI_COMM_WORLD);
            //printf("Node %d will receive %d elements\n", i, matrices.a_prime.row_pos[end + 1] - 1 - matrices.a_prime.row_pos[start]);
            mpi_send_items(matrices.a_prime.items, matrices.a_prime.total_items, i, 0, MPI_COMM_WORLD);
            mpi_send_items(matrices.a_prime_transpose.items, matrices.a_prime_transpose.total_items, i, 0, MPI_COMM_WORLD);
            MPI_Send(matrices.l.data, matrices.l.rows*matrices.l.columns, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
            MPI_Send(matrices.r.data, matrices.r.rows*matrices.r.columns, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
        }
        /*
        for(int i = 0; i < matrices.l.columns; i++){
            printf("Line from node 0, pos %d has value %f\n",i, matrices.l.data[i]);
        }
        */



        /*
        int lines_per_node_prime = matrices.l.n_rows / nprocs;
        int lines_per_node_transpose = matrices.r.n_rows / nprocs;
        int prime_rest = matrices.a_prime.n_rows % nprocs;
        int transpose_rest = matrices.a_prime_transpose.n_rows % nprocs;
        printf("Number of lines %d\n", matrices.a_prime.n_rows);
        printf("Lines per node %d\n", lines_per_node_prime);
        printf("Lines for node 0: start-%d end-%d\n", lines_per_node_prime * (nprocs - 1), matrices.l.n_rows - 1);
        for(int i = 1; i < nprocs; i++){
            int start = lines_per_node_prime * (i - 1);
            int end = lines_per_node_prime*i - 1;
            printf("Lines for node %d: start-%d end-%d\n", i, start, end);
            int n_elems = matrices.l.row_pos[end + 1] - 1 - matrices.l.row_pos[start];
            MPI_Send(&n_elems, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            //printf("Node %d will receive %d elements\n", i, matrices.a_prime.row_pos[end + 1] - 1 - matrices.a_prime.row_pos[start]);
        }
        */
    } else {
        size_t send_size_t[11];
        MPI_Recv(send_size_t, 11, my_MPI_SIZE_T, 0, 0, MPI_COMM_WORLD, NULL);
        matrices.num_iterations = send_size_t[0];
        matrices.a_prime.total_items = send_size_t[1];
        matrices.a_prime.n_rows = send_size_t[2];
        matrices.a_prime.n_cols = send_size_t[3];
        matrices.a_prime_transpose.total_items = send_size_t[4];
        matrices.a_prime_transpose.n_rows = send_size_t[5];
        matrices.a_prime_transpose.n_cols = send_size_t[6];
        matrices.l.rows = send_size_t[7];
        matrices.l.columns = send_size_t[8];
        matrices.r.rows = send_size_t[9];
        matrices.r.columns = send_size_t[10];
        //printf("Num of iterations %zu\n", matrices.num_iterations);
        //printf("Matrix l num of rows %zu \n", matrices.l.rows);

        matrices.a_prime.items = malloc(sizeof(Item) * matrices.a_prime.total_items);
        matrices.a_prime_transpose.items = malloc(sizeof(Item) * matrices.a_prime_transpose.total_items);

        mpi_recv_items(matrices.a_prime.items, matrices.a_prime.total_items, 0, 0, MPI_COMM_WORLD, NULL);
        mpi_recv_items(matrices.a_prime_transpose.items, matrices.a_prime_transpose.total_items, 0, 0, MPI_COMM_WORLD, NULL);

        matrices.l.data = malloc(sizeof(double) * (matrices.l.rows * matrices.l.columns));
        matrices.r.data = malloc(sizeof(double) * (matrices.r.rows * matrices.r.columns));

        MPI_Recv(matrices.l.data, matrices.l.rows*matrices.l.columns, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, NULL);
        MPI_Recv(matrices.r.data, matrices.r.rows*matrices.r.columns, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, NULL);
        /*
        if(me == 1){
            for(int i = 0; i < matrices.l.columns; i++){
                printf("Line from node 1, pos %d has value %f\n",i, matrices.l.data[i]);
            }
        }
        */

        /*
        int n_elems;
        MPI_Recv(&n_elems, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);
        printf("Node %d receives %d elems\n", me, n_elems);
        */
    }
    /*
    for(int i = 0; i < matrices.a_prime.n_rows; i++){
        printf("Row %d starts at position %ld\n", i, matrices.a_prime.row_pos[i]);
    }
    */

    MPI_Finalize();
    return 0;
}
