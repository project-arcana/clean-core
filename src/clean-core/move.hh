#pragma once

#include <clean-core/detail/remove_reference.hh>
#include <clean-core/macros.hh>

namespace cc
{
template <class T>
CC_FORCE_INLINE constexpr remove_reference<T>&& move(T&& t) noexcept
{
    return static_cast<remove_reference<T>&&>(t);
}
}
