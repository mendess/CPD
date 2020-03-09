#include "matFact.h"
#include <stdio.h>
#include "parser.h"

inline double delta(double const A, double const B, double const LR){
    return 2 * (A - B) * (-LR);
}

Matrix matrix_b(Matrix const* L, Matrix const* R) {
    Matrix matrix = matrix_make(L->rows, R->columns);
    for (size_t i = 0; i < L->rows; i++) {
        for (size_t j = 0; j < R->columns; ++j) {
            for (size_t k = 0; k < L->columns; ++k) {
                *matrix_at_mut(&matrix, i, j) +=
                    *matrix_at(L, i, k) * *matrix_at(R, k, j);
            }
        }
    }
    return matrix;
}

/*
void next_iter(Matrix *A, Matrix *L, Matrix *R, double const alpha){
    Matrix B = matrix_b(L, R);
    Matrix aux_L, aux_R;
    aux_L = matrix_clone(L);
    aux_R = matrix_clone(R);
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
*/

Matrix next_iterL(Matrixes const *matrixes){
    Matrix aux_L;
    aux_L = matrix_clone(&matrixes->l);
    Matrix B = matrix_b(&matrixes->l, &matrixes->r);
    for(size_t i = 0; i < matrixes->l.rows; i++){
        for(size_t k= 0; k < matrixes->l.columns; k++){
            double aux = 0;
            for(size_t j = 0; j < matrixes->a.columns; j++){
                if(*matrix_at(&matrixes->a, i, j) !=0){
                    aux += delta(*matrix_at(&matrixes->a, i,j), *matrix_at(&B, i, j), *matrix_at(&matrixes->r, k, j));
                }
            }
            *matrix_at_mut(&aux_L, i, k) = *matrix_at(&matrixes->l, i, k) - matrixes->alpha * aux;
        }
    }
    return aux_L;
}

Matrix next_iterR(Matrixes const *matrixes){
    Matrix aux_R;
    aux_R = matrix_clone(&matrixes->r);
    Matrix B = matrix_b(&matrixes->l, &matrixes->r);
    for(size_t k = 0; k < matrixes->r.rows; k++){
        for(size_t j= 0; j < matrixes->r.columns; j++){
            double aux = 0;
            for(size_t i = 0; i < matrixes->a.rows; i++){
                if(*matrix_at(&matrixes->a, i, j) !=0){
                    aux += delta(*matrix_at(&matrixes->a, i,j), *matrix_at(&B, i, j), *matrix_at(&matrixes->l, i, k));
                }
            }
            *matrix_at_mut(&aux_R, k, j) = *matrix_at(&matrixes->r, k, j) - matrixes->alpha * aux;
        }
    }
    matrix_free(&B);
    return aux_R;
}

void iter(Matrixes *matrixes){
    for(size_t i = 0; i < matrixes->num_iterations; i++){
        Matrix aux_L = next_iterL(matrixes);
        Matrix aux_R = next_iterR(matrixes);
        matrix_free(&matrixes->l);
        matrix_free(&matrixes->r);
        matrixes->l = aux_L;
        matrixes->r = aux_R;
    }
}