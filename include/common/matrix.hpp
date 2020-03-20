#ifndef MATRIX_H
#define MATRIX_H
#define _DEFAULT_SOURCE
#include "compact_matrix.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

namespace matrix {
struct Matrix {
    using IteratorMut = std::vector<double>::iterator;
    using Iterator = std::vector<double>::const_iterator;

  private:
    size_t _rows;
    size_t _columns;
    std::vector<double> _data;

    struct Row;
    struct RowMut;

  public:
    Matrix(size_t rows, size_t columns)
        : _rows(rows), _columns(columns), _data(rows * columns) {}

    explicit Matrix(Matrix const&) noexcept = delete;

    Matrix(Matrix&&) noexcept = default;

    auto constexpr n_rows() const noexcept -> size_t { return _rows; }

    auto constexpr n_columns() const noexcept -> size_t { return _columns; }

    auto operator=(Matrix const&) -> Matrix& = delete;

    auto operator=(Matrix &&) -> Matrix& = default;

    auto clear() noexcept -> void {
        std::fill(_data.begin(), _data.end(), 0.0);
    }

    auto friend operator<<(std::ostream& os, Matrix const&) -> std::ostream&;

    auto operator[](std::pair<size_t, size_t> i) const -> double const& {
        auto& [row, column] = i;
        assert(_rows > row);
        assert(_columns > column);
        return _data[row * _columns + column];
    }

    auto operator[](std::pair<size_t, size_t> i) -> double& {
        auto& [row, column] = i;
        assert(_rows > row);
        assert(_columns > column);
        return _data[row * _columns + column];
    }

    auto begin() noexcept -> IteratorMut { return _data.begin(); }

    auto cbegin() const noexcept -> Iterator { return _data.cbegin(); }

    auto end() noexcept -> IteratorMut { return _data.end(); }

    auto cend() const noexcept -> Iterator { return _data.cend(); }

    auto row_mut(size_t row) noexcept -> RowMut { return RowMut{*this, row}; }

    auto row(size_t row) const noexcept -> Row { return Row{*this, row}; }

  private:
    struct RowMut {
      private:
        IteratorMut _it;
        Iterator const _end;

      public:
        RowMut(Matrix& m, size_t row)
            : _it(m._data.begin() + row * m._columns),
              _end(m._data.begin() + (row + 1) * m._columns) {}

        auto operator++() noexcept -> RowMut& { // ++row == row.operator++()
            ++_it;
            return *this;
        }

        auto operator*() noexcept -> double& { return *_it; }

        auto has_next() noexcept -> bool { return _it != _end; }
    };

    struct Row {
      private:
        Iterator _it;
        Iterator const _end;

      public:
        Row(Matrix const& m, size_t row)
            : _it(m._data.begin() + row * m._columns),
              _end(m._data.begin() + (row + 1) * m._columns) {}

        auto operator++() noexcept -> Row& {
            ++_it;
            return *this;
        }

        auto operator*() noexcept -> double const& { return *_it; }

        auto has_next() noexcept -> bool { return _it != _end; }
    };
};

struct Matrices {
    size_t const num_iterations;
    double const alpha;
    CompactMatrix const a;
    CompactMatrix const a_transposed;
    Matrix l;
    Matrix r;

    Matrices(
        size_t num_iterations,
        double alpha,
        CompactMatrix&& a,
        CompactMatrix&& a_transposed,
        Matrix&& l,
        Matrix&& r)
        : num_iterations(num_iterations),
          alpha(alpha),
          a(std::move(a)),
          a_transposed(std::move(a_transposed)),
          l(std::move(l)),
          r(std::move(r)) {}

    auto output(Matrix const& b) -> void {
        double max;
        size_t max_pos;
        for (size_t row = 0; row < a.n_rows(); row++) {
            max = 0;
            max_pos = 0;
            // TODO: Use a better iterator
            for (size_t column = 0; column < a.n_columns(); column++) {
                if (a[std::pair(row, column)] == 0) {
                    double aux = b[std::pair(row, column)];
                    if (aux > max) {
                        max = aux;
                        max_pos = column;
                    }
                }
            }
            std::cout << max_pos << '\n';
        }
    }
};

auto random_fill_LR(size_t nF, Matrix& l, Matrix& r) -> void;

} // namespace matrix

#endif // MATRIX_H
