#pragma once

#include <clean-core/detail/remove_reference.hh>
#include <clean-core/macros.hh>

namespace cc
{
template <class T>
CC_FORCE_INLINE constexpr T&& forward(remove_reference<T>& t) noexcept
{
    return static_cast<T&&>(t);
}
template <class T>
CC_FORCE_INLINE constexpr T&& forward(remove_reference<T>&& t) noexcept
{
    return static_cast<T&&>(t);
}
}
