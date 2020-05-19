#ifndef MPI_WRAPPERS
#define MPI_WRAPPERS

#include "common/compact_matrix.h"

#include <limits.h>
#include <mpi.h>
#include <stdint.h>

#if SIZE_MAX == UCHAR_MAX
#    define MPI_SIZE_T MPI_UNSIGNED_CHAR
#elif SIZE_MAX == USHRT_MAX
#    define MPI_SIZE_T MPI_UNSIGNED_SHORT
#elif SIZE_MAX == UINT_MAX
#    define MPI_SIZE_T MPI_UNSIGNED
#elif SIZE_MAX == ULONG_MAX
#    define MPI_SIZE_T MPI_UNSIGNED_LONG
#elif SIZE_MAX == ULLONG_MAX
#    define MPI_SIZE_T MPI_UNSIGNED_LONG_LONG
#else
#    error "what is happening here?"
#endif

extern MPI_Comm WORLD_COMM;

typedef enum { ROOT = 0 } Node;

void mpi_send_size(size_t size, Node dest);

size_t mpi_recv_size(Node from);

void mpi_send_doubles(double* buf, int len, Node dest);

void mpi_recv_doubles(double* buf, int len, Node from);

void mpi_send_items(Item const*, int len, Node dest);

void mpi_recv_items(Item* items, int len, Node source);

#endif
