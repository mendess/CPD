#include "cmatrix.h"
#include "mpi_size_t.h"
#include "parser.h"

#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef LEN
#    define LEN 2
#endif

int main(int argc, char** argv) {
    int me, nprocs;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &me);


    if (me == 0) {
        printf("LEN is = %d\n", LEN);
        Item items[LEN];
        for (size_t i = 0; i < LEN; ++i) {
            items[i].row = i;
            items[i].column = i * 2;
            items[i].value = i * 3.14;
        }
        for (int p = 1; p < nprocs; ++p) {
            mpi_send_items(items, LEN, p, 0, MPI_COMM_WORLD);
            printf("Sent to %d\n", p);
        }
        printf("Done, sleeping\n");
    } else {
        Item items[LEN];
        MPI_Status status;
        printf("I'm %d and I'm gonna get some\n", me);
        mpi_recv_items(items, LEN, 0, 0, MPI_COMM_WORLD, &status);
        printf("I'm %d and I got some\n", me);
        for (size_t i = 0; i < LEN; ++i) {
            assert(items[i].row == i);
            assert(items[i].column == i * 2);
            assert(items[i].value == i * 3.14);
        }
    }

    MPI_Finalize();
    return 0;
}
