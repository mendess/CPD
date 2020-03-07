#include "matFact.h"
#include <stdio.h>
double delta(double const A, double const B, double const LR){
    return 2 * (A - B) * (-LR);
}

void next_iter(Matrix *A, Matrix *L, Matrix *R, double const alpha){
    Matrix B = matrix_b(L, R);
    Matrix aux_L, aux_R;
    aux_L = matrix_clone(L);
    aux_R = matrix_clone(R);
    puts("Clone matrix R");
    matrix_print(&aux_R);
    for(size_t row = 0; row < A->rows; row++){
        for(size_t column = 0; column < A->columns; column++){
            if(*matrix_at(A, row, column)!=0){
                for(size_t k = 0; k < L->columns; k++){
                    double aux=0;
                    for(size_t j = 0; j < A->rows; j++){
                        if(*matrix_at(A,row,j)!=0)
                            aux += delta(*matrix_at(A,row,j), *matrix_at(&B,row, j), *matrix_at(&aux_R,k,j));
                    }
                    *matrix_at_mut(L, row, k) = *matrix_at(&aux_L, row, k) - alpha*aux;
                }
                for(size_t k = 0; k < A->columns; k++){
                    double aux=0;
                    for(size_t i = 0; i < L->rows; i++){
                        if(*matrix_at(A,i,column)!=0)
                            aux += delta(*matrix_at(A,i,column), *matrix_at(&B,i, column), *matrix_at(&aux_L,i,column));
                    }
                    *matrix_at_mut(R, k, column) = *matrix_at(&aux_R, k, column) - alpha*aux;
                }
            }
        }
    }
}