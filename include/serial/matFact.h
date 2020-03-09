#ifndef MATFACT_H
#define MATFACT_H
#include "matrix.h"
#include "parser.h"
double delta(double const A, double const B, double const LR);

void next_iter(Matrix *A, Matrix *L, Matrix *R, double const alpha);
Matrix next_iterL(Matrixes const *);
Matrix next_iterR(Matrixes const *);
void iter(Matrixes *);
#endif 