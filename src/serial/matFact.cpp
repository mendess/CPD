#include "matFact.hpp"

#include "compact_matrix.hpp"
#include "matrix.hpp"
#include "parser.hpp"
using namespace matrix;

namespace factorization {

#define DELTA(a, b, lr) (2 * ((a) - (b)) * (-lr))

/* constexpr inline double delta(double const a, double const b, double const
 * lr) { */
/*     return 2 * (a - b) * (-lr); */
/* } */

void matrix_b(Matrix const& l, Matrix const& r, Matrix& matrix) {
    for (size_t i = 0; i < l.n_rows(); i++) {
        for (size_t j = 0; j < r.n_columns(); ++j) {
            for (size_t k = 0; k < l.n_columns(); ++k) {
                matrix[std::pair(i, j)] +=
                    l[std::pair(i, k)] * r[std::pair(k, j)];
            }
        }
    }
}

void next_iter_l(Matrices const& matrices, Matrix& aux_l, Matrix const& b) {
    for (size_t i = 0; i < matrices.l.n_rows(); i++) {
        for (size_t k = 0; k < matrices.l.n_columns(); k++) {
            double aux = 0;
            auto& a = matrices.a;
            for (auto a_iter = a.row(i); a_iter.has_next(); ++a_iter) {
                auto [value, j] = *a_iter;
                aux += DELTA(
                    value, b[std::pair(i, j)], matrices.r[std::pair(k, j)]);
            }
            aux_l[std::pair(i, k)] =
                matrices.l[std::pair(i, k)] - matrices.alpha * aux;
        }
    }
}

void next_iter_r(Matrices const& matrices, Matrix& aux_r, Matrix const& b) {
    for (size_t k = 0; k < matrices.r.n_rows(); k++) {
        for (size_t j = 0; j < matrices.r.n_columns(); j++) {
            double aux = 0;
            auto& a = matrices.a_transposed;
            for (auto a_iter = a.row(j); a_iter.has_next(); ++a_iter) {
                auto [value, i] = *a_iter;
                aux += DELTA(
                    value, b[std::pair(i, j)], matrices.l[std::pair(i, k)]);
            }
            aux_r[std::pair(k, j)] =
                matrices.r[std::pair(k, j)] - matrices.alpha * aux;
        }
    }
}

auto iter(Matrices& matrices) -> Matrix {
    auto aux_l = Matrix(matrices.l.n_rows(), matrices.l.n_columns());
    auto aux_r = Matrix(matrices.r.n_rows(), matrices.r.n_columns());
    auto b = Matrix(matrices.a.n_rows(), matrices.a.n_columns());
    for (size_t i = 0; i < matrices.num_iterations; i++) {
        if (i != 0) b.clear(); // TODO: benchmark
        matrix_b(matrices.l, matrices.r, b);
        next_iter_l(matrices, aux_l, b);
        next_iter_r(matrices, aux_r, b);
        std::swap(matrices.l, aux_l);
        std::swap(matrices.r, aux_r);
    }
    return b;
}
} // namespace factorization
