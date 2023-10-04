#pragma once

#include <clean-core/has_operator.hh>

#include <clean-core/fwd.hh>

namespace cc
{
template <class T, class>
struct less
{
    static_assert(has_operator_less<T>, "if no bool-valued operator< is found, provide a specialization less<>");

    [[nodiscard]] constexpr bool operator()(T const& a, T const& b) const noexcept { return a < b; }
};

template <>
struct less<void> // transparent comparison
{
    template <class A, class B>
    [[nodiscard]] constexpr bool operator()(A const& a, B const& b) const noexcept
    {
        return a < b;
    }
};

template <class T, class>
struct greater
{
    static_assert(has_operator_greater<T>, "if no bool-valued operator< is found, provide a specialization greater<>");

    [[nodiscard]] constexpr bool operator()(T const& a, T const& b) const noexcept { return a > b; }
};

template <>
struct greater<void> // transparent comparison
{
    template <class A, class B>
    [[nodiscard]] constexpr bool operator()(A const& a, B const& b) const noexcept
    {
        return a > b;
    }
};

/// helper for implementing total orders for tuple-like types
///
/// example:
///
///   struct foo
///   {
///       int x;
///       int y;
///       bool operator<(foo const& r) const {
///           // implements lexicographic order on (x, y)
///           return cc::cascaded_less(
///               x, r.x,
///               y, r.y
///           );
///       }
///   };
///
/// NOTE: always uses op< and op== internally
///
/// TODO: use spaceship in c++20
template <class Lhs, class Rhs, class... Rest>
[[nodiscard]] constexpr bool cascaded_less(Lhs const& lhs, Rhs const& rhs, Rest const&... rest)
{
    static_assert(sizeof...(Rest) % 2 == 0, "must provide an even number of arguments");
    if constexpr (sizeof...(Rest) == 0)
        return lhs < rhs;
    else
        return lhs != rhs ? lhs < rhs : cc::cascaded_less(rest...);
}

} // namespace cc
