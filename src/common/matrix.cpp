#include "matrix.hpp"

#include <cassert>
#include <cstdlib>
#include <iomanip>

namespace matrix {

#define RAND01 ((double) random() / (double) RAND_MAX)

auto operator<<(std::ostream& os, Matrix const& m) -> std::ostream& {
    for (size_t r = 0; r < m.n_rows(); ++r) {
        for (auto i = m.row(r); i.has_next(); ++i) {
            os << std::fixed << std::setprecision(6) << *i << ' ';
        }
        os << '\n';
    }
    return os;
}

auto random_fill_LR(size_t const nF, Matrix& l, Matrix& r) -> void {
    srandom(0);
    for (auto& v : l) {
        v = RAND01 / (double) nF;
    }
    for (auto& v : r) {
        v = RAND01 / (double) nF;
    }
}
} // namespace matrix
