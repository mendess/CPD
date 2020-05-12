#ifndef MATFACT_H
#define MATFACT_H
#include "common/matrix.h"

Matrix iter_mpi(Matrices*, int nprocs, int me, size_t nk);

extern int G_ME;

#endif
