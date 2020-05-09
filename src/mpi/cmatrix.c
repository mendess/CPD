#include "common/compact_matrix.h"
#include "common/debug.h"

#include <mpi.h>

void mpi_send_items(
    Item const* const items, int len, int dest, int tag, MPI_Comm comm) {
    unsigned char* buf = (unsigned char*) items;
    int buf_len = len * (int) sizeof(Item);
    if (MPI_Send(buf, buf_len, MPI_UNSIGNED_CHAR, dest, tag, comm)) {
        debug_print_backtrace("couldn't send items");
    }
}

void mpi_recv_items(
    Item* items,
    int len,
    int source,
    int tag,
    MPI_Comm comm,
    MPI_Status* status) {
    unsigned char* buf = (unsigned char*) items;
    int buf_len = len * (int) sizeof(Item);
    if (MPI_Recv(buf, buf_len, MPI_UNSIGNED_CHAR, source, tag, comm, status)) {
        debug_print_backtrace("couldn't receive items");
    }
}
