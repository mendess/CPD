#ifndef RESULT_T
#define RESULT_T

#include <optional>
#include <stdexcept>
#include <variant>

namespace result {
template<typename T, typename E>
class Result {
  private:
    std::variant<T, E> inner;

  public:
    constexpr Result(std::variant<T, E> inner): inner(std::move(inner)) {}
    constexpr Result(T&& t): inner(std::forward<T>(t)) {}
    constexpr Result(E&& t): inner(std::forward<E>(t)) {}

    constexpr static auto Ok(T&& t) -> Result {
        return Result{
            std::variant<T, E>(std::in_place_index<0>, std::forward<T>(t))};
    }

    constexpr static auto Err(E&& e) -> Result {
        return Result{
            std::variant<T, E>(std::in_place_index<1>, std::forward<E>(e))};
    }

    constexpr Result(const Result<T, E>&) = default;

    constexpr Result(Result<T, E>&&) = default;

    auto constexpr operator=(const Result<T, E>&) -> Result<T, E>& = default;

    auto constexpr operator=(Result<T, E> &&) -> Result<T, E>& = default;

    auto constexpr is_ok() const noexcept -> bool { return inner.index() == 0; }

    auto constexpr is_err() const noexcept -> bool {
        return inner.index() == 1;
    }

    auto constexpr ok() && -> std::optional<T> {
        if (is_ok()) {
            return {std::get<0>(inner)};
        } else {
            return {};
        }
    }

    auto constexpr err() && -> std::optional<E> {
        if (is_err()) {
            return {std::get<1>(inner)};
        } else {
            return {};
        }
    }

    auto constexpr as_ref() const noexcept -> Result<T const*, E const*> {
        if (is_ok()) {
            return Result<T const*, E const*>::Ok(&std::get<0>(inner));
        } else {
            return Result<T const*, E const*>::Err(&std::get<1>(inner));
        }
    }

    auto constexpr as_mut() noexcept -> Result<T*, E*> {
        if (is_ok()) {
            return Result<T*, E*>::Ok(&std::get<0>(inner));
        } else {
            return Result<T*, E*>::Err(&std::get<1>(inner));
        }
    }

    template<typename U, typename M> // M: T -> U
        auto constexpr map(M mapper) && noexcept -> Result<U, E> {
        if (is_ok()) {
            return Result<U, E>::Ok(
                mapper(std::forward<T>(std::get<0>(inner))));
        } else {
            return Result<U, E>::Err(std::forward<E>(std::get<1>(this->inner)));
        }
    }

    template<typename U, typename M, typename F> // M: T -> U; F: E -> U
        auto constexpr map_or_else(F fallback, M mapper) && noexcept -> U {
        if (is_ok()) {
            return mapper(std::get<0>(inner));
        } else {
            return fallback(std::get<1>(inner));
        }
    }

    template<typename F, typename O>
        auto constexpr map_err(O op) && noexcept -> Result<T, F> {
        if (is_ok()) {
            return Result<T, F>::Ok(std::forward<T>(std::get<0>(this->inner)));
        } else {
            return Result<T, F>::Err(op(std::forward<E>(std::get<1>(inner))));
        }
    }

    auto constexpr unwrap() && -> T { return std::move(std::get<0>(inner)); }

    auto constexpr unwrap_or(T&& optb) && noexcept -> T {
        if (is_ok()) {
            return std::get<0>(inner);
        } else {
            return optb;
        }
    }

    template<typename F>
        auto constexpr unwrap_or_else(F op) && noexcept -> T {
        if (is_ok()) {
            return std::get<0>(inner);
        } else {
            return op(std::get<1>(inner));
        }
    }

    auto constexpr unwrap_err() && -> E {
        return std::move(std::get<1>(inner));
    }

    auto expect(const std::string& msg) && -> T {
        try {
            return unwrap();
        } catch (...) {
            throw std::runtime_error(msg);
        }
    }

    auto expect_err(const std::string& msg) && -> T {
        try {
            return unwrap_err();
        } catch (...) {
            throw std::runtime_error(msg);
        }
    }

    auto constexpr unwrap_or_default() && noexcept -> T {
        if (is_ok()) {
            return std::get<0>(inner);
        } else {
            return T();
        }
    }

    template<typename U>
        auto constexpr andd(Result<U, E> res) && noexcept -> Result<U, E> {
        if (is_ok()) {
            return res;
        } else {
            return Result<U, E>::Err(std::move(std::get<1>(inner)));
        }
    }

    template<typename U, typename M> // M: T -> Result<U, E>
        auto constexpr and_then(M mapper) && noexcept -> Result<U, E> {
        if (is_ok()) {
            return mapper(std::get<0>(inner));
        } else {
            return Result<U, E>::Err(std::move(std::get<1>(inner)));
        }
    }

    template<typename F>
        auto constexpr orr(Result<T, F> res) && noexcept -> Result<T, F> {
        if (is_ok()) {
            return Result<T, F>::Ok(std::move(std::get<0>(inner)));
        } else {
            return res;
        }
    }

    template<typename F, typename O> // O: E -> Result<T, F>
        auto constexpr or_else(O op) && noexcept -> Result<T, F> {
        if (is_ok()) {
            return Result<T, F>::Ok(std::move(std::get<0>(inner)));
        } else {
            return op(std::get<1>(inner));
        }
    }

    auto constexpr operator==(const Result<T, E>& other) noexcept -> bool {
        return inner == other.inner;
    }

    auto constexpr operator!=(const Result<T, E>& other) noexcept -> bool {
        return inner != other.inner;
    }

    auto constexpr operator<(const Result<T, E>& other) noexcept -> bool {
        return inner < other.inner;
    }

    auto constexpr operator>(const Result<T, E>& other) noexcept -> bool {
        return inner > other.inner;
    }

    auto constexpr operator<=(const Result<T, E>& other) noexcept -> bool {
        return inner <= other.inner;
    }

    auto constexpr operator>=(const Result<T, E>& other) noexcept -> bool {
        return inner >= other.inner;
    }

    auto friend operator<<(std::ostream& os, Result<T, E> const& r) {
        return r.is_ok() ? os << "Ok(" << std::get<0>(r.inner) << ")"
                         : os << "Err(" << std::get<1>(r.inner) << ")";
    }
};

template<typename F, typename T, typename E>
auto attempt(F f) -> Result<T, E> {
    try {
        return Result<T, E>::Ok(f());
    } catch (E& e) {
        return Result<T, E>::Err(e);
    }
}

template<typename F, typename T, typename E>
auto attempt(F f, E e) -> Result<T, E> {
    try {
        return Result<T, E>::Ok(f());
    } catch (...) {
        return Result<T, E>::Err(e);
    }
}

template<typename T, typename E>
auto Ok(T&& t) -> Result<T, E> {
    return Result<T, E>::Err(std::forward<T>(t));
}

template<typename T, typename E>
auto Err(E&& e) -> Result<T, E> {
    return Result<T, E>::Err(std::forward<E>(e));
}

struct Unit {
    Unit() {}
};

} // namespace result

#endif /* RESULT_T */
