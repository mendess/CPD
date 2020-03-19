#include "compact_matrix.hpp"

#include <iomanip>

namespace matrix {
auto operator<<(std::ostream& os, CompactMatrix const& m) -> std::ostream& {
    for (size_t r = 0; r < m.n_rows(); ++r) {
        for (size_t c = 0; c < m.n_columns(); ++c) {
            os << std::fixed << std::setprecision(6) << m[std::pair(r, c)]
               << ' ';
        }
        os << '\n';
    }
    return os;
}
} // namespace matrix
