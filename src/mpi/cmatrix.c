#include "common/compact_matrix.h"

#include <mpi.h>

int mpi_send_items(
    Item const* const items, int len, int dest, int tag, MPI_Comm comm) {
    unsigned char* buf = (unsigned char*) items;
    int buf_len = len * (int) sizeof(Item);
    return MPI_Send(buf, buf_len, MPI_UNSIGNED_CHAR, dest, tag, comm);
}

int mpi_recv_items(
    Item* items,
    int len,
    int source,
    int tag,
    MPI_Comm comm,
    MPI_Status* status) {
    unsigned char* buf = (unsigned char*) items;
    int buf_len = len * (int) sizeof(Item);
    return MPI_Recv(buf, buf_len, MPI_UNSIGNED_CHAR, source, tag, comm, status);
}
