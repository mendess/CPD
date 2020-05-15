#ifndef MPI_CMATRIX_H
#define MPI_CMATRIX_H

#include "common/matrix.h"

#include <mpi.h>

void random_fill_LT_R_mpi(Matrix* l, Matrix* r, int me, int nprocs, size_t k);

void random_fill_L_RT_mpi(Matrix* l, Matrix* r, int me, int nprocs, size_t k);

void random_fill_LR_parts(Matrix* l, Matrix* r, CompactMatrix const* a);

#endif // MPI_CMATRIX_H
