#ifndef MATFACT_H
#define MATFACT_H
#include "matrix.h"
double delta(double const A, double const B, double const LR);

void next_iter(Matrix *A, Matrix *L, Matrix *R, double const alpha);
#endif 