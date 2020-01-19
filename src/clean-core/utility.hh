#pragma once

#include <clean-core/assert.hh>
#include <clean-core/typedefs.hh>

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
[[nodiscard]] inline constexpr size_t wrapped_increment(size_t pos, size_t max)
{
    ++pos;
    return pos == max ? 0 : pos;
}
}
