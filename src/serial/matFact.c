#include "matFact.h"
#include <stdio.h>
double delta(double const A, double const B, double const LR){
    return 2 * (A - B) * (-LR);
}

void next_iter(Matrix *A, Matrix *L, Matrix *R, double const alpha){
    Matrix B = matrix_b(L, R);
    puts("Matrix B begin of next_iter");
    matrix_print(&B);
    for(size_t row = 0; row < A->rows; row++){
        for(size_t column = 0; column < A->columns; column++){
            if(*matrix_at(A, row, column)!=0){
                for(size_t k = 0; k < L->columns; k++){
                    double aux=0;
                    for(size_t j = 0; j < R->rows; j++){
                        if(*matrix_at(A,row,j)!=0)
                            aux += delta(*matrix_at(A,row,j), *matrix_at(&B,row, j), *matrix_at(R,k,j));
                    }
                    *matrix_at_mut(L, row, k) = *matrix_at(L, row, k) - alpha*aux;
                    printf("Alpha %lf\n", alpha);
                }
                for(size_t k = 0; k < R->columns; k++){
                    double aux=0;
                    for(size_t i = 0; i < L->rows; i++){
                        if(*matrix_at(A,i,column)!=0)
                            aux += delta(*matrix_at(A,i,column), *matrix_at(&B,i, column), *matrix_at(L,i,column));
                    }
                    *matrix_at_mut(R, k, column) = *matrix_at(R, k, column) - alpha*aux;
                }
            }
        }
    }
    B = matrix_b(L,R);
    puts("Matrix B end next_iter \n");
    matrix_print(&B);
}