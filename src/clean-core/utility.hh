#pragma once

namespace cc
{
template <class T>
constexpr T const& max(T const& a, T const& b)
{
    return (a < b) ? b : a;
}

template <class T>
constexpr T const& min(T const& a, T const& b)
{
    return (a < b) ? a : b;
}
}
