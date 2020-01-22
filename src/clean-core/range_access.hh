#pragma once

namespace cc
{
template <class ContainerT>
inline constexpr auto begin(ContainerT& c) -> decltype(c.begin())
{
    return c.begin();
}

template <class ContainerT>
inline constexpr auto begin(ContainerT const& c) -> decltype(c.begin())
{
    return c.begin();
}

template <class ContainerT>
inline constexpr auto end(ContainerT& c) -> decltype(c.end())
{
    return c.end();
}

template <class ContainerT>
inline constexpr auto end(ContainerT const& c) -> decltype(c.end())
{
    return c.end();
}

template <class T, int N>
inline constexpr T* begin(T (&array)[N]){
    return array;
}

template <class T, int N>
inline constexpr T* end(T (&array)[N]){
    return array + N;
}
}
