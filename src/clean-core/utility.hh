#pragma once

#include <cstddef> // size_t

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

/// Divide ints and round up, nom > 0, denom > 0
template <class T>
[[nodiscard]] constexpr T int_div_ceil(T nom, T denom)
{
    CC_CONTRACT(nom > 0 && denom > 0);
    return 1 + ((nom - 1) / denom);
}

/// Ceil a value to a multiple of a given value
template <class T>
[[nodiscard]] constexpr T int_ceil_to_multiple(T val, T multiple)
{
    return ((val + multiple - 1) / multiple) * multiple;
}

[[maybe_unused]] struct
{
    template <class T>
    constexpr void operator()(T& a, T& b) const
    {
        _external_cc_detail::do_swap(a, b, 0);
    }
} constexpr swap; // implemented as functor so it cannot be found by ADL

// straightforward swap that does not respect custom overloads
template <class T>
constexpr void simple_swap(T& a, T& b)
{
    T tmp = static_cast<T&&>(a);
    a = static_cast<T&&>(b);
    b = static_cast<T&&>(tmp);
}

/// increment the value (pointer or integer) to align to the given mask
template <class T>
[[nodiscard]] T align_up_masked(T value, size_t mask)
{
    return (T)(((size_t)value + mask) & ~mask);
}

/// decrement the value (pointer or integer) to align to the given mask
template <class T>
[[nodiscard]] T align_down_masked(T value, size_t mask)
{
    return (T)((size_t)value & ~mask);
}

/// increment the value (pointer or integer) to align at the given boundary
/// use with ints: align_up(300, 16) = 304
/// or with ptrs: align_up(0x5ACE, 256) = 0x5B00
/// alignment must be a power of 2
template <class T>
[[nodiscard]] T align_up(T value, size_t alignment)
{
    return align_up_masked(value, alignment - 1);
}

/// decrement the value (pointer or integer) to align at the given boundary
/// use with ints: align_down(300, 16) = 288
/// or with ptrs: align_down(0x5ACE, 256) = 0x5A00
/// alignment must be a power of 2
template <class T>
[[nodiscard]] T align_down(T value, size_t alignment)
{
    return align_down_masked(value, alignment - 1);
}

/// returns true if the value (pointer or integer) is aligned at the given boundary
template <class T>
[[nodiscard]] constexpr bool is_aligned(T value, size_t alignment)
{
    return 0 == ((size_t)value & (alignment - 1));
}
}
