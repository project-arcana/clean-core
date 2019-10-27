#pragma once

#include <clean-core/detail/remove_reference.hh>

namespace cc
{
template <class T>
constexpr T&& forward(remove_reference<T>& t) noexcept
{
    return static_cast<T&&>(t);
}
template <class T>
constexpr T&& forward(remove_reference<T>&& t) noexcept
{
    return static_cast<T&&>(t);
}
}
