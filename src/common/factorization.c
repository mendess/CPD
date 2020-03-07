#include "factorization.h"
double delta(double const A, double const B, double const LR){
    return 2 * (A - B) * (-LR);
}

/*
double sumL(Matrix *A, Matrix *L, Matrix *R, double const alpha){
    for(int row = 0; row < A.rows; row++){
        for(int column = 0; column < A.columns; column++){
            double* aux = matrix_at(A, row, column);
            if(*aux!=0){

            }
        }
    }
}
*/