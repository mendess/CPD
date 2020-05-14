#include "mpi/wrappers.h"

#include "common/compact_matrix.h"
#include "common/debug.h"
#include "common/matrix.h"

#include <mpi.h>

void mpi_send_size(size_t const size, Node const dest) {
    if (MPI_Send(&size, 1, MPI_SIZE_T, dest, 0, MPI_COMM_WORLD)) {
        eprintln("Couldn't send size '%zu' to proc %d", size, dest);
        debug_print_backtrace("couldn't send buffer");
    }
}

void mpi_send_doubles(double* const buf, int const len, Node const dest) {
    if (MPI_Send(buf, len, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD)) {
        eprintln("Couldn't send buffer of size %d to proc %d", len, dest);
        debug_print_backtrace("couldn't send buffer");
    }
}

size_t mpi_recv_size(Node const from) {
    size_t size;
    if (MPI_Recv(&size, 1, MPI_SIZE_T, from, 0, MPI_COMM_WORLD, NULL)) {
        eprintln("Couldn't recv size from proc %d", from);
        debug_print_backtrace("couldn't send buffer");
    }
    return size;
}

void mpi_recv_doubles(double* const buf, int const len, Node const from) {
    if (MPI_Recv(buf, len, MPI_DOUBLE, from, 0, MPI_COMM_WORLD, NULL)) {
        eprintln("Couldn't recv buffer of size %d from proc %d", len, from);
        debug_print_backtrace("couldn't send buffer");
    }
}

void mpi_send_items(Item const* const items, int const len, Node const dest) {

    unsigned char* buf = (unsigned char*) items;
    int buf_len = len * (int) sizeof(Item);
    if (MPI_Send(buf, buf_len, MPI_UNSIGNED_CHAR, dest, 0, MPI_COMM_WORLD)) {
        debug_print_backtrace("couldn't send items");
    }
}

void mpi_recv_items(Item* const items, int const len, Node const source) {

    unsigned char* buf = (unsigned char*) items;
    int buf_len = len * (int) sizeof(Item);
    if (MPI_Recv(
            buf, buf_len, MPI_UNSIGNED_CHAR, source, 0, MPI_COMM_WORLD, NULL)) {
        debug_print_backtrace("couldn't receive items");
    }
}

void cmatrix_bcast_items(Item* const items, int const len, Node const source) {
    unsigned char* buf = (unsigned char*) items;
    int buf_len = len * (int) sizeof(Item);
    if (MPI_Bcast(buf, buf_len, MPI_UNSIGNED_CHAR, source, MPI_COMM_WORLD)) {
        debug_print_backtrace("couldn't broadcast items");
    }
}
