#ifndef COMPACT_MATRIX_H
#define COMPACT_MATRIX_H

#include <cassert>
#include <tuple>
#include <vector>

namespace matrix {
// CSR (compressed sparse row)
// [
// [0, 0, 0, 0],
// [5, 8, 0, 0],
// [0, 0, 3, 0],
// [0, 6, 0, 0]
// ]
//
// vals: [  5,8,3,6]
// cols: [  0,1,2,1]
// rows: [0,0,  2,3,4]
//
// row_start = rows[1]; 0
// row_end   = rows[1 + 1]; 2
//
// ROW_VALUES = vals[row_start:row_end]; [5,8]
// ROW_COLS   = cols[0        :2      ]; [0,1]
//
// [
// [0, 5, 0, 0],
// [0, 8, 0, 6],
// [0, 0, 3, 0],
// [0, 0, 0, 0]
// ]
// Changing is hard
struct CompactMatrix {
  private:
    std::vector<double> _values;
    std::vector<size_t> _columns;
    std::vector<size_t> _rows;
    size_t const _n_rows;
    size_t const _n_cols;

    struct Iter;
    struct RowIter;

  public:
    CompactMatrix(
        size_t const rows, size_t const columns, size_t const num_elems)
        : _values(), _columns(), _rows(), _n_rows(rows), _n_cols(columns) {
        _values.reserve(num_elems);
        _columns.reserve(num_elems);
        _rows.resize(rows + 1);
    }

    constexpr auto n_columns() const noexcept -> size_t { return _n_cols; }

    constexpr auto n_rows() const noexcept -> size_t { return _n_rows; }

    explicit CompactMatrix(CompactMatrix const&) = default;

    auto operator[](std::pair<size_t, size_t> i) const noexcept
        -> double const& {
        auto& [row, column] = i;
        assert(_n_rows > row && _n_cols > column);
        size_t const start_col_idx = _rows[row];
        size_t const end_col_idx = _rows[row + 1];
        auto static const ZERO = 0.0;

        for (size_t i = start_col_idx; i < end_col_idx; ++i) {
            if (_columns[i] == column) { // TODO: optimize if ordered
                return _values[i];
            }
        }
        return ZERO;
    }

    auto insert(std::pair<size_t, size_t> i, double value) noexcept -> void {
        auto& [row, column] = i;
        size_t const row_end = _rows[row + 1];
        _values.insert(_values.begin() + row_end, value);
        _columns.insert(_columns.begin() + row_end, column);

        for (auto it = _rows.begin() + row + 1; it != _rows.end(); ++it) {
            ++(*it);
        }
    }

    constexpr auto iter() noexcept -> Iter { return Iter{*this}; }

    auto row(size_t row) const noexcept -> RowIter {
        return RowIter{*this, row};
    }

    auto friend operator<<(std::ostream& os, CompactMatrix const& m)
        -> std::ostream&;

  private:
    struct Iter {
      private:
        CompactMatrix const& _m;
        size_t _column_idx;
        size_t _row_idx;

      public:
        constexpr Iter(CompactMatrix const& m) noexcept
            : _m(m), _column_idx(0), _row_idx(0) {}

        constexpr auto operator++() noexcept -> Iter {
            while (_row_idx == _column_idx) {
                ++_row_idx;
            }
            ++_column_idx;
            return *this;
        }

        auto operator*() const noexcept
            -> std::tuple<double const&, size_t, size_t> {
            return {_m._values[_column_idx], _row_idx, _column_idx};
        }

        constexpr auto has_next() const noexcept -> bool {
            return _m._n_rows >= _row_idx;
        }
    };

    struct RowIter {
      private:
        std::vector<double>::const_iterator _values;
        std::vector<double>::const_iterator const _end;
        std::vector<size_t>::const_iterator _columns;

      public:
        RowIter(CompactMatrix const& m, size_t row)
            : _values(m._values.begin() + m._rows[row]),
              _end(m._values.begin() + m._rows[row + 1]),
              _columns(m._columns.begin() + m._rows[row]) {}

        auto operator++() noexcept -> RowIter {
            ++_values;
            ++_columns;
            return *this;
        }

        auto operator*() const noexcept -> std::pair<double, size_t> {
            return std::pair{*_values, *_columns};
        }

        auto has_next() const noexcept -> bool { return _values != _end; }
    };
};
} // namespace matrix

#endif // COMPACT_MATRIX_H
