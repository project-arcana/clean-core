#pragma once

#include <clean-core/assert.hh>

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
}
