#ifndef MATFACT_H
#define MATFACT_H
#include "common/matrix.h"

Matrix iter_mpi(Matrices*, int nprocs, int me);

extern int G_ME;

#endif
