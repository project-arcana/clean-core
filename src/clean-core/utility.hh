#pragma once

#include <clean-core/assert.hh>

namespace _external_cc_detail
{
template <class T>
constexpr void do_swap(T& a, T& b, char) // priority 0
{
    T tmp = static_cast<T&&>(a);
    a = static_cast<T&&>(b);
    b = static_cast<T&&>(tmp);
}
template <class T>
constexpr auto do_swap(T& a, T& b, int) -> decltype(swap(a, b)) // priority 1
{
    swap(a, b);
}
}

namespace cc
{
template <class T>
[[nodiscard]] constexpr T const& max(T const& a, T const& b)
{
    return (a < b) ? b : a;
}

template <class T>
[[nodiscard]] constexpr T const& min(T const& a, T const& b)
{
    return (a < b) ? a : b;
}

template <class T>
[[nodiscard]] constexpr T const& clamp(T const& v, T const& lo, T const& hi)
{
    CC_CONTRACT(!(hi < lo));
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

// optimal assembly for increment with custom wrap-around
// https://godbolt.org/z/rTklbk
// (assembly only tested for integral types)
template <class T>
[[nodiscard]] constexpr T wrapped_increment(T pos, T max)
{
    ++pos;
    return pos == max ? 0 : pos;
}

template <class T>
[[nodiscard]] constexpr T wrapped_decrement(T pos, T max)
{
    CC_CONTRACT(max > 0);
    return pos == 0 ? max - 1 : pos - 1;
}

struct
{
    template <class T>
    constexpr void operator()(T& a, T& b) const
    {
        _external_cc_detail::do_swap(a, b, 0);
    }
} constexpr swap; // implemented as functor so it cannot be found by ADL

// Divide ints and round up
// a > 0, b > 0
template <class T>
[[nodiscard]] constexpr T int_div_ceil(T a, T b)
{
    return 1 + ((a - 1) / b);
}
}
