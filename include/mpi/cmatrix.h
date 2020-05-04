#ifndef MPI_CMATRIX_H
#define MPI_CMATRIX_H

#include "compact_matrix.h"

#include <mpi.h>

int mpi_send_items(
    Item const* const, int len, int dest, int tag, MPI_Comm comm);

int mpi_recv_items(
    Item* items,
    int len,
    int source,
    int tag,
    MPI_Comm comm,
    MPI_Status* status);

#endif // MPI_CMATRIX_H
