#include "parser.hpp"

#include "compact_matrix.hpp"
#include "result.hpp"

#include <cstdio>
#include <fstream>
#include <optional>
#include <string>

namespace parser {
using namespace result;
using namespace matrix;

struct StrIter {
  private:
    char const* _str;

  public:
    StrIter(std::string& s) noexcept: _str(s.c_str()) {}

    constexpr auto str() const noexcept -> char const* { return _str; }

    auto skip_line() noexcept -> void {
        if (*_str == '\0') return;
        while (*_str != '\n') {
            if (*_str == '\0') return;
            ++_str;
        }
        ++_str;
    }
};

struct Header {
  public:
    size_t features;
    size_t users;
    size_t items;
    size_t non_zero_elems;
    size_t num_iterations;
    double alpha;

    auto friend operator<<(std::ostream& os, Header const& h) -> std::ostream& {
        return os << "Header{"
                  << " features: " << h.features << ", users: " << h.users
                  << ", items: " << h.items
                  << ", non_zero_elems: " << h.non_zero_elems
                  << ", num_iterations: " << h.num_iterations
                  << ", alpha: " << h.alpha << " }";
    }
};

auto read_file(char const* filename) -> std::optional<std::string> {
    auto in = std::ifstream{filename};
    if (in) {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        return contents;
    }
    return std::nullopt;
}

template<typename... Args>
auto scan_line(StrIter& s_iter, char const* const format, Args... args) -> int {
    int const formats_read = std::sscanf(s_iter.str(), format, args...);
    s_iter.skip_line();
    return formats_read;
}

auto parse_header(StrIter& iter) -> Result<Header, ParserError> {
    size_t num_iterations = 0;
    if (scan_line(iter, "%zu\n", &num_iterations) != 1) {
        std::cerr << "Failed to get number of iterations\n";
        return ParserError::INVALID_FORMAT;
    }
    double alpha;
    if (scan_line(iter, "%lf", &alpha) != 1) {
        std::cerr << "Failed to get alpha\n";
        return ParserError::INVALID_FORMAT;
    }
    size_t features;
    if (scan_line(iter, "%zu", &features) != 1) {
        std::cerr << "Failed to get number of features\n";
        return ParserError::INVALID_FORMAT;
    }
    size_t users, items, non_zero_elems;
    if (scan_line(iter, "%zu %zu %zu", &users, &items, &non_zero_elems) != 3) {
        std::cerr << "Failed to get matrix A information\n";
        return ParserError::INVALID_FORMAT;
    }
    return Header{
        features,
        users,
        items,
        non_zero_elems,
        num_iterations,
        alpha,
    };
}

using CMatrices = std::pair<CompactMatrix, CompactMatrix>;

auto parse_matrix_a(
    StrIter& iter,
    size_t const non_zero_elems,
    size_t const rows,
    size_t const columns) -> Result<CMatrices, ParserError> {
    auto a = CompactMatrix(rows, columns, non_zero_elems);
    auto a_transpose = CompactMatrix(columns, rows, non_zero_elems);
    size_t row, column;
    double value;
    size_t n_lines = 0;
    while (scan_line(iter, "%zu %zu %lf", &row, &column, &value) == 3) {
        ++n_lines;
        if (row >= rows || column >= columns) {
            std::cerr << "Invalid row or column values at line " << n_lines
                      << ". Max (" << rows << ", " << columns << "), got ("
                      << row << ", " << columns << ")\n";
            return ParserError::INVALID_FORMAT;
        } else if (n_lines > non_zero_elems) {
            std::cerr << "More elements than expected\n";
            return ParserError::INVALID_FORMAT;
        } else if (0.0 > value || value > 5.0) {
            std::cerr << "Invalid matrix value at line " << n_lines << ": "
                      << value << "\n";
            return ParserError::INVALID_FORMAT;
        }
        a.insert({row, column}, value);
        a_transpose.insert({column, row}, value);
    }

    if (n_lines < non_zero_elems) {
        std::cerr << "Not as many elements as expected\n";
        return ParserError::INVALID_FORMAT;
    }

    return CMatrices(a, a_transpose);
}

auto parse(char const* filename) -> Result<Matrices, ParserError> {
    auto contents = read_file(filename);
    if (!contents) return ParserError::IO;

    auto content_iter = StrIter{*contents};
    auto maybe_header = parse_header(content_iter);
    if (maybe_header.is_err()) {
        return std::move(maybe_header).unwrap_err();
    }
    auto header = std::move(maybe_header).unwrap();

    auto maybe_matrices = parse_matrix_a(
        content_iter, header.non_zero_elems, header.users, header.items);

    if (maybe_matrices.is_err()) {
        return std::move(maybe_matrices).unwrap_err();
    }

    auto l = Matrix(header.users, header.features);
    auto r = Matrix(header.features, header.items);
    random_fill_LR(header.features, l, r);
    auto as = std::move(maybe_matrices).unwrap();
    return Matrices{header.num_iterations,
                    header.alpha,
                    std::move(std::get<0>(as)),
                    std::move(std::get<1>(as)),
                    std::move(l),
                    std::move(r)};
}
} // namespace parser
