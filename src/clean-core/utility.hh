#pragma once

namespace cc
{
template <class T>
constexpr T const& max(const T& a, const T& b)
{
    return (a < b) ? b : a;
}

template <class T>
constexpr T const& min(const T& a, const T& b)
{
    return (a < b) ? a : b;
}
}
