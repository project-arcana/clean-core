#pragma once

#include <cc/detail/traits/remove_reference.hh>

namespace cc
{
template <class T>
[[nodiscard]] constexpr auto move(T&& t) noexcept -> remove_reference<T>&&
{
    return static_cast<remove_reference<T>&&>(t);
}

template <class T>
[[nodiscard]] constexpr auto forward(remove_reference<T>& t) noexcept -> T&&
{
    return static_cast<T&&>(t);
}

template <class T>
[[nodiscard]] constexpr auto forward(remove_reference<T>&& t) noexcept -> T&&
{
    return static_cast<T&&>(t);
}
} // namespace cc
