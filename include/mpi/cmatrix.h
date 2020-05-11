#ifndef MPI_CMATRIX_H
#define MPI_CMATRIX_H

#include "common/compact_matrix.h"
#include "common/matrix.h"

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

void cmatrix_bcast_items(Item* items, int len, int source);

void random_fill_LT_R_mpi(
    Matrix* const l,
    Matrix* const r,
    int const me,
    int const nprocs,
    size_t const k);

#endif // MPI_CMATRIX_H
