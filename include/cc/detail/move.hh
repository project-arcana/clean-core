#pragma once

#include <cc/detail/traits/remove_reference.hh>

namespace cc
{
template <class T>
constexpr auto move(T&& t) noexcept
{
    return static_cast<remove_reference<T>&&>(t);
}

template <class T>
constexpr auto forward(remove_reference<T>& t) noexcept
{
    return static_cast<T&&>(t);
}

template <class T>
constexpr auto forward(remove_reference<T>&& t) noexcept
{
    return static_cast<T&&>(t);
}
} // namespace cc
